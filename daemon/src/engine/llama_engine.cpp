#include "llama_engine.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonValue>
#include <QLoggingCategory>

#include <algorithm>
#include <chat.h>
#include <common.h>
#include <ggml-backend.h>
#include <llama.h>
#include <mutex>
#include <sampling.h>
#include <speculative.h>
#include <string>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcEngine, "paraphrase.engine")

namespace paraphrase::engine {
namespace {

using Tokens = std::vector<llama_token>;

constexpr int kBatch = 2048; // llama-server's defaults (common_params)
constexpr int kUbatch = 512;

/// llama.cpp logs through this: warnings and errors to our category, the rest dropped. It
/// never sees user text (prompts are not logged by llama.cpp at these levels).
void llamaLog(ggml_log_level level, const char* text, void* /*user*/)
{
    if (level == GGML_LOG_LEVEL_ERROR) {
        qCWarning(lcEngine).noquote() << "llama.cpp:" << QString::fromUtf8(text).trimmed();
    }
}

/// Portable builds (ADR-008) load one CPU library per instruction-set level at run time and
/// keep the best: from $PARAPHRASE_GGML_BACKEND_DIR (development builds, where they sit in
/// the build's bin/), else from `<prefix>/lib/paraphrase/ggml` beside the daemon (packages),
/// else where llama.cpp looks itself (the program's folder). Native builds have the CPU
/// backend built in, and none of this applies.
void initBackend()
{
    static std::once_flag once;
    std::call_once(once, [] {
        llama_log_set(llamaLog, nullptr);
        if (ggml_backend_reg_count() == 0) {
            QString dir = qEnvironmentVariable("PARAPHRASE_GGML_BACKEND_DIR");
            if (dir.isEmpty()) {
                dir = QCoreApplication::applicationDirPath() + u"/../lib/paraphrase/ggml"_s;
            }
            if (QFileInfo(dir).isDir()) {
                ggml_backend_load_all_from_path(QFile::encodeName(dir).constData());
            }
        }
        llama_backend_init(); // loads from the default places when nothing is loaded yet
        qCInfo(lcEngine) << "backends:" << ggml_backend_reg_count();
    });
}

/// Length of the longest prefix of `bytes` that does not end inside a UTF-8 sequence.
size_t completeUtf8(const std::string& bytes)
{
    const size_t n = bytes.size();
    for (size_t back = 1; back <= 4 && back <= n; ++back) {
        const auto c = static_cast<unsigned char>(bytes[n - back]);
        if ((c & 0xC0U) == 0x80U) {
            continue; // a continuation byte: keep looking for the lead byte
        }
        size_t need = 1;
        if ((c & 0xE0U) == 0xC0U) {
            need = 2;
        } else if ((c & 0xF0U) == 0xE0U) {
            need = 3;
        } else if ((c & 0xF8U) == 0xF0U) {
            need = 4;
        }
        return need <= back ? n : n - back;
    }
    return n;
}

/// The skill's sampling on top of the model's defaults, as llama-server reads a request.
common_params_sampling samplingFor(const common_params_sampling& defaults, const QJsonObject& s)
{
    common_params_sampling p = defaults;
    const auto number = [&s](const QString& key) { return s.value(key); };
    if (const QJsonValue v = number(u"temperature"_s); v.isDouble()) {
        p.temp = static_cast<float>(v.toDouble());
    }
    if (const QJsonValue v = number(u"top_p"_s); v.isDouble()) {
        p.top_p = static_cast<float>(v.toDouble());
    }
    if (const QJsonValue v = number(u"top_k"_s); v.isDouble()) {
        p.top_k = v.toInt();
    }
    if (const QJsonValue v = number(u"min_p"_s); v.isDouble()) {
        p.min_p = static_cast<float>(v.toDouble());
    }
    if (const QJsonValue v = number(u"repeat_penalty"_s); v.isDouble()) {
        p.penalty_repeat = static_cast<float>(v.toDouble());
    }
    if (const QJsonValue v = number(u"seed"_s); v.isDouble()) {
        p.seed = static_cast<uint32_t>(v.toInteger());
    }
    return p;
}

/// Template parts may hold control tokens; the user's parts are plain text (untrusted).
Tokens tokenize(const llama_vocab* vocab, const common_chat_prompt& prompt)
{
    Tokens out;
    bool first = true;
    for (const auto& part : prompt.parts) {
        const Tokens t =
            common_tokenize(vocab, part.text, /*add_special=*/first, /*parse_special=*/!part.is_input);
        out.insert(out.end(), t.begin(), t.end());
        first = false;
    }
    return out;
}

common_chat_templates_inputs chatInputs(const GenerationRequest& request)
{
    common_chat_templates_inputs in;
    common_chat_msg system;
    system.role = "system";
    system.content = request.system.toStdString();
    common_chat_msg user;
    user.role = "user";
    user.content = request.user.toStdString();
    in.messages = {system, user};
    in.use_jinja = true;
    in.add_generation_prompt = true;
    in.reasoning_format = COMMON_REASONING_FORMAT_DEEPSEEK;
    in.enable_thinking = false; // PLAN D12, as llama-server --reasoning off
    in.chat_template_kwargs["enable_thinking"] = "false";
    return in;
}

/// Decodes `tokens` from `pos` on in sequence `seq`, logits only for the last one.
bool decodePrompt(llama_context* ctx, const Tokens& tokens, size_t from, llama_seq_id seq)
{
    llama_batch batch = llama_batch_init(kBatch, 0, 1);
    bool ok = true;
    for (size_t i = from; i < tokens.size() && ok;) {
        batch.n_tokens = 0;
        for (; i < tokens.size() && batch.n_tokens < kBatch; ++i) {
            const auto k = static_cast<size_t>(batch.n_tokens++);
            batch.token[k] = tokens[i];
            batch.pos[k] = static_cast<llama_pos>(i);
            batch.n_seq_id[k] = 1;
            batch.seq_id[k][0] = seq;
            batch.logits[k] = static_cast<int8_t>(i + 1 == tokens.size());
        }
        ok = llama_decode(ctx, batch) == 0;
    }
    llama_batch_free(batch);
    return ok;
}

} // namespace

/// One generate() call: the prompt, the samplers and the state of every variant.
class LlamaEngine::Run
{
public:
    Run(LlamaEngine& engine, const GenerationRequest& request, const TextSink& sink)
        : m_engine(engine)
        , m_request(request)
        , m_sink(sink)
        , m_ctx(engine.m_init->context())
        , m_vocab(llama_model_get_vocab(engine.m_init->model()))
        , m_session(engine.m_templates.get(), m_vocab, chatInputs(request))
        // Drafting decodes one sequence: a request for several variants is not drafted.
        , m_drafted(request.draft && request.variants <= 1 && engine.canDraft())
    {
        for (const auto& stop : m_session.additional_stops()) {
            m_longestStop = std::max(m_longestStop, stop.size());
        }
    }

    util::Result<Generation> execute();
    /// Reads the prompt into the cache and stops there (LlamaEngine::warm).
    util::Result<bool> prefill()
    {
        Generation unused;
        return preparePrompt(unused);
    }

private:
    struct Variant
    {
        common_sampler_ptr sampler;
        std::string bytes;   ///< everything generated
        size_t streamed = 0; ///< bytes already passed to the sink
        llama_pos pos = 0;
        int batchIndex = -1; ///< where its logits are in the last batch
        bool done = false;
        int tokens = 0;
    };

    util::Result<bool> preparePrompt(Generation& out);
    [[nodiscard]] bool decodeDraftedPrompt(const Tokens& prompt, size_t from);
    void prepareVariants(const Tokens& prompt);
    bool step(Generation& out);
    bool draftStep(Generation& out);
    bool take(size_t index, llama_token tok, Generation& out);
    void emit(int index, bool final);
    [[nodiscard]] size_t stopAt(const std::string& bytes) const;

    LlamaEngine& m_engine;
    const GenerationRequest& m_request;
    const TextSink& m_sink;
    llama_context* m_ctx;
    const llama_vocab* m_vocab;
    common_chat_session m_session;
    bool m_drafted;
    std::vector<Variant> m_variants;
    size_t m_longestStop = 0;
    QElapsedTimer m_clock;

    // Drafted runs only, as llama.cpp's speculative-simple example: the tokens in the cache,
    // the last token (not yet decoded) and its position.
    Tokens m_context;
    llama_token m_last = LLAMA_TOKEN_NULL;
    llama_pos m_past = 0;
    Tokens m_draft;
};

namespace {

size_t commonPrefix(const std::vector<int>& cached, const Tokens& prompt)
{
    size_t n = 0;
    while (n < cached.size() && n < prompt.size() && cached[n] == prompt[n]) {
        ++n;
    }
    return n;
}

} // namespace

util::Result<bool> LlamaEngine::Run::preparePrompt(Generation& out)
{
    const Tokens prompt = tokenize(m_vocab, m_session.prompt());
    if (static_cast<int>(prompt.size()) + m_request.maxTokens > m_engine.m_config.contextSize) {
        return util::Result<bool>::failure(
            QCoreApplication::translate("LlamaEngine", "The text is too long for this model (%1 tokens).")
                .arg(prompt.size()));
    }
    // Reuse the longest prefix already in the cache, as llama-server's prompt cache does; the
    // last prompt token is always decoded again for its logits. A drafted run needs the prefix
    // in the draft context too, which only drafted runs fill.
    size_t keep = commonPrefix(m_engine.m_cachedTokens, prompt);
    if (m_drafted) {
        keep = std::min(keep, commonPrefix(m_engine.m_draftCachedTokens, prompt));
    }
    keep = std::min(keep, prompt.size() - 1);
    llama_memory_t memory = llama_get_memory(m_ctx);
    llama_memory_t draftMemory = m_drafted ? llama_get_memory(m_engine.m_draftInit->context()) : nullptr;
    const auto from = static_cast<llama_pos>(keep);
    if (!llama_memory_seq_rm(memory, 0, from, -1) ||
        (draftMemory != nullptr && !llama_memory_seq_rm(draftMemory, 0, from, -1))) {
        llama_memory_seq_rm(memory, 0, -1, -1);
        if (draftMemory != nullptr) {
            llama_memory_seq_rm(draftMemory, 0, -1, -1);
        }
        keep = 0;
    }
    for (int s = 1; s < m_engine.m_config.maxVariants; ++s) {
        llama_memory_seq_rm(memory, s, -1, -1);
    }
    out.promptTokens = static_cast<int>(prompt.size());
    out.cachedTokens = static_cast<int>(keep);
    m_engine.m_cachedTokens.clear();
    if (m_drafted) {
        m_engine.m_draftCachedTokens.clear();
        if (!decodeDraftedPrompt(prompt, keep)) {
            return util::Result<bool>::failure(
                QCoreApplication::translate("LlamaEngine", "The model could not read the text."));
        }
        // The last prompt token is decoded with the first draft.
        m_engine.m_cachedTokens.assign(prompt.begin(), prompt.end() - 1);
        m_engine.m_draftCachedTokens = m_engine.m_cachedTokens;
    } else {
        if (!decodePrompt(m_ctx, prompt, keep, 0)) {
            return util::Result<bool>::failure(
                QCoreApplication::translate("LlamaEngine", "The model could not read the text."));
        }
        m_engine.m_cachedTokens.assign(prompt.begin(), prompt.end());
    }
    prepareVariants(prompt);
    return util::Result<bool>::success(true);
}

bool LlamaEngine::Run::decodeDraftedPrompt(const Tokens& prompt, size_t from)
{
    common_speculative* spec = m_engine.m_spec.get();
    common_batch batch(m_ctx);
    const size_t end = prompt.size() - 1;
    for (size_t i = from; i < end;) {
        batch.clear();
        for (; i < end && batch.size() < kBatch; ++i) {
            batch.add(prompt[i], static_cast<llama_pos>(i), 0, false);
        }
        // The draft head reads the model's state for every token the model decodes.
        if (llama_process(m_ctx, LLAMA_PROCESS_TYPE_DECODE, batch.get()) != 0 ||
            !common_speculative_process(spec, batch)) {
            return false;
        }
    }
    m_context.assign(prompt.begin(), prompt.end() - 1);
    m_last = prompt.back();
    m_past = static_cast<llama_pos>(end);
    common_speculative_begin(spec, 0, m_context);
    return true;
}

void LlamaEngine::Run::prepareVariants(const Tokens& prompt)
{
    common_params_sampling params = samplingFor(*m_engine.m_defaults, m_request.sampling);
    if (!m_request.grammar.isEmpty()) {
        params.grammar = common_grammar(COMMON_GRAMMAR_TYPE_USER, m_request.grammar.toStdString());
    }
    m_session.apply_sampling(params);
    const int n = m_drafted ? 1 : std::clamp(m_request.variants, 1, m_engine.m_config.maxVariants);
    m_variants.resize(static_cast<size_t>(n));
    for (int v = 0; v < n; ++v) {
        common_params_sampling own = params;
        if (v > 0 && own.seed != LLAMA_DEFAULT_SEED) {
            own.seed += static_cast<uint32_t>(v); // each variant its own seed, as llama-server
        }
        if (v > 0) {
            llama_memory_seq_cp(llama_get_memory(m_ctx), 0, v, -1, -1);
        }
        auto& var = m_variants[static_cast<size_t>(v)];
        var.sampler.reset(common_sampler_init(m_engine.m_init->model(), own));
        // As llama-server: the sampler sees the prompt, so the repeat penalty counts its words.
        for (const llama_token tok : prompt) {
            common_sampler_accept(var.sampler.get(), tok, false);
        }
        var.pos = static_cast<llama_pos>(prompt.size());
        var.batchIndex = -1; // the prompt's last token: logits index -1
    }
}

size_t LlamaEngine::Run::stopAt(const std::string& bytes) const
{
    size_t at = std::string::npos;
    for (const auto& stop : m_session.additional_stops()) {
        if (!stop.empty()) {
            at = std::min(at, bytes.find(stop));
        }
    }
    return at;
}

void LlamaEngine::Run::emit(int index, bool final)
{
    auto& var = m_variants[static_cast<size_t>(index)];
    size_t limit = var.bytes.size();
    if (!final) {
        // Hold back what could still become a stop string, then whole characters only.
        const size_t hold = m_longestStop > 0 ? m_longestStop - 1 : 0;
        limit = limit > hold ? limit - hold : 0;
    }
    limit = completeUtf8(var.bytes.substr(0, limit));
    if (limit > var.streamed && m_sink) {
        m_sink(index, QString::fromUtf8(var.bytes.data() + var.streamed,
                                        static_cast<qsizetype>(limit - var.streamed)));
    }
    var.streamed = std::max(var.streamed, limit);
}

/// One sampled token of a variant: false when the variant is done (end, stop string, limit).
bool LlamaEngine::Run::take(size_t index, llama_token tok, Generation& out)
{
    auto& var = m_variants[index];
    ++var.tokens;
    ++out.generatedTokens;
    if (llama_vocab_is_eog(m_vocab, tok)) {
        var.done = true;
        return false;
    }
    var.bytes += common_token_to_piece(m_ctx, tok, false);
    if (out.ttftSeconds == 0.0 && completeUtf8(var.bytes) > 0) {
        out.ttftSeconds = static_cast<double>(m_clock.nsecsElapsed()) / 1e9;
    }
    if (const size_t at = stopAt(var.bytes); at != std::string::npos) {
        var.bytes.resize(at);
        var.done = true;
        return false;
    }
    if (var.tokens >= m_request.maxTokens) {
        var.done = true;
        out.finishReason = u"length"_s;
        return false;
    }
    emit(static_cast<int>(index), false);
    return true;
}

bool LlamaEngine::Run::step(Generation& out)
{
    llama_batch batch = llama_batch_init(static_cast<int32_t>(m_variants.size()), 0, 1);
    for (size_t v = 0; v < m_variants.size(); ++v) {
        auto& var = m_variants[v];
        if (var.done) {
            continue;
        }
        const llama_token tok = common_sampler_sample(var.sampler.get(), m_ctx, var.batchIndex);
        common_sampler_accept(var.sampler.get(), tok, true);
        if (!take(v, tok, out)) {
            continue;
        }
        const auto k = static_cast<size_t>(batch.n_tokens);
        batch.token[k] = tok;
        batch.pos[k] = var.pos++;
        batch.n_seq_id[k] = 1;
        batch.seq_id[k][0] = static_cast<llama_seq_id>(v);
        batch.logits[k] = 1;
        var.batchIndex = batch.n_tokens++;
    }
    const bool more = batch.n_tokens > 0 && !m_engine.m_cancel.load();
    const bool ok = !more || llama_decode(m_ctx, batch) == 0;
    llama_batch_free(batch);
    return ok && more;
}

/// One round of drafting: the head proposes a few tokens, the model decodes the last token
/// and the proposals in one batch, and keeps the proposals its own sampler agrees with.
/// Greedy answers are the same as without drafting.
bool LlamaEngine::Run::draftStep(Generation& out)
{
    const auto& var = m_variants.front();
    if (var.done || m_engine.m_cancel.load()) {
        return false;
    }
    common_speculative* spec = m_engine.m_spec.get();
    const llama_context* draftCtx = m_engine.m_draftInit->context();
    const int budget = std::min(m_engine.m_config.contextSize - static_cast<int>(m_past) - 2,
                                m_request.maxTokens - var.tokens - 1);
    m_draft.clear();
    if (budget > 0) {
        auto& params = common_speculative_get_draft_params(spec, 0);
        params.drafting = true;
        params.n_max = budget;
        params.pos0 = m_past;
        params.id_last = m_last;
        params.prompt = &m_context;
        params.result = &m_draft;
        params.result_q = nullptr;
        common_speculative_draft(spec);
        llama_memory_seq_rm(llama_get_memory(draftCtx), 0, m_past, -1);
    }

    common_batch batch(m_ctx);
    batch.add(m_last, m_past, 0, true);
    for (size_t i = 0; i < m_draft.size(); ++i) {
        batch.add(m_draft[i], m_past + 1 + static_cast<llama_pos>(i), 0, true);
    }
    if (llama_process(m_ctx, LLAMA_PROCESS_TYPE_DECODE, batch.get()) != 0 ||
        !common_speculative_process(spec, batch)) {
        return false;
    }
    const Tokens ids = common_sampler_sample_and_accept_n(var.sampler.get(), m_ctx, m_draft);
    const auto accepted = static_cast<uint16_t>(ids.size() - 1);
    common_speculative_accept(spec, 0, accepted);
    out.draftTokens += static_cast<int>(m_draft.size());
    out.draftAccepted += accepted;

    bool more = true;
    for (const llama_token id : ids) {
        m_context.push_back(m_last);
        m_last = id;
        if (!take(0, id, out)) {
            more = false;
            break;
        }
    }
    m_past += static_cast<llama_pos>(ids.size());
    // Drop the rejected proposals from both caches.
    llama_memory_seq_rm(llama_get_memory(m_ctx), 0, m_past, -1);
    llama_memory_seq_rm(llama_get_memory(draftCtx), 0, m_past, -1);
    return more;
}

util::Result<Generation> LlamaEngine::Run::execute()
{
    m_clock.start();
    Generation out;
    out.finishReason = u"stop"_s;
    if (const auto ready = preparePrompt(out); !ready.ok()) {
        return util::Result<Generation>::failure(ready.error());
    }
    if (m_drafted) {
        while (draftStep(out)) {
        }
    } else {
        while (step(out)) {
        }
    }
    if (m_engine.m_cancel.load()) {
        out.finishReason = u"cancelled"_s;
    }
    for (size_t v = 0; v < m_variants.size(); ++v) {
        emit(static_cast<int>(v), true);
        // The chat session extracts the answer from the raw text, as llama-server does.
        common_chat_session parser = m_session;
        const common_chat_msg& msg = parser.finish(common_chat_input(m_variants[v].bytes));
        out.texts.append(QString::fromStdString(msg.content));
    }
    // Generated tokens are not tracked in the prompt cache: the next prompt shares only the
    // system prompt and the start of the user message with this one.
    out.totalSeconds = static_cast<double>(m_clock.nsecsElapsed()) / 1e9;
    qCDebug(lcEngine) << "generated" << out.generatedTokens << "tokens," << out.promptTokens
                      << "prompt tokens," << out.cachedTokens << "cached," << out.draftAccepted << "of"
                      << out.draftTokens << "drafted," << out.totalSeconds << "s," << out.finishReason;
    return util::Result<Generation>::success(out);
}

void LlamaEngine::TemplatesDeleter::operator()(common_chat_templates* tmpls) const
{
    common_chat_templates_free(tmpls);
}

void LlamaEngine::SpeculativeDeleter::operator()(common_speculative* spec) const
{
    common_speculative_free(spec);
}

LlamaEngine::LlamaEngine() = default;
LlamaEngine::~LlamaEngine() = default;

util::Result<bool> LlamaEngine::load(const EngineConfig& config)
{
    initBackend();
    QElapsedTimer clock;
    clock.start();
    unload();
    common_params params;
    params.model.path = config.modelPath.toStdString();
    params.n_ctx = config.contextSize * config.maxVariants;
    params.n_batch = kBatch;
    params.n_ubatch = kUbatch;
    params.n_parallel = config.maxVariants;
    params.kv_unified = config.maxVariants > 1;
    params.swa_full = true; // any cached prefix can be reused (see the class comment)
    params.n_gpu_layers = config.gpuLayers;
    params.flash_attn_type =
        config.flashAttention ? LLAMA_FLASH_ATTN_TYPE_ENABLED : LLAMA_FLASH_ATTN_TYPE_DISABLED;
    if (config.threads > 0) {
        params.cpuparams.n_threads = config.threads;
        params.cpuparams_batch.n_threads = config.threads;
    }
    // What llama-server's argument parser does: -1 threads becomes the number of math cores.
    postprocess_cpu_params(params.cpuparams, nullptr);
    postprocess_cpu_params(params.cpuparams_batch, &params.cpuparams);
    if (!config.draftModelPath.isEmpty()) {
        // As llama-server with `--spec-type draft-mtp -md <head>` and the default draft length;
        // the model's context must hold the outputs of a drafted batch.
        params.speculative.types = {COMMON_SPECULATIVE_TYPE_DRAFT_MTP};
        params.speculative.draft.mparams.path = config.draftModelPath.toStdString();
        const auto limits = common_speculative_get_output_limits(
            params.n_batch, params.n_parallel, common_speculative_n_max(&params.speculative));
        params.n_outputs_max = std::max(1, limits.total);
        params.n_outputs_max_per_seq = std::max(1, limits.per_seq);
    }
    auto init = common_init_from_params(params);
    if (!init || init->model() == nullptr || init->context() == nullptr) {
        return util::Result<bool>::failure(
            QCoreApplication::translate("LlamaEngine", "The model could not be loaded from %1.")
                .arg(config.modelPath));
    }
    m_templates.reset(common_chat_templates_init(init->model(), "").release());
    m_defaults = std::make_unique<common_params_sampling>(params.sampling);
    m_init = std::move(init);
    m_config = config;
    m_cachedTokens.clear();
    m_draftCachedTokens.clear();
    if (!config.draftModelPath.isEmpty()) {
        loadDraft(config, params);
    }
    m_loadSeconds = static_cast<double>(clock.nsecsElapsed()) / 1e9;
    qCInfo(lcEngine) << "model loaded in" << m_loadSeconds << "s, drafting" << (canDraft() ? "on" : "off");
    return util::Result<bool>::success(true);
}

/// The draft head and its context, as llama-server creates them. Without them the engine
/// still works, only slower, so a problem is logged and drafting stays off.
void LlamaEngine::loadDraft(const EngineConfig& config, common_params& params)
{
    llama_context* ctx = m_init->context();
    common_params draftParams = common_base_params_to_speculative(params);
    auto draftInit = common_speculative_init_from_params(draftParams, m_init->model(), ctx);
    if (!draftInit || draftInit->context() == nullptr) {
        qCWarning(lcEngine) << "draft head not loaded, drafting off:" << config.draftModelPath;
        return;
    }
    // The drafted loop trims rejected proposals from both caches.
    if (common_context_can_seq_rm(ctx) != COMMON_CONTEXT_SEQ_RM_TYPE_PART ||
        common_context_can_seq_rm(draftInit->context()) != COMMON_CONTEXT_SEQ_RM_TYPE_PART) {
        qCWarning(lcEngine) << "this model's cache cannot drop drafted tokens, drafting off";
        return;
    }
    params.speculative.draft.ctx_tgt = ctx;
    params.speculative.draft.ctx_dft = draftInit->context();
    common_speculative* spec = common_speculative_init(params.speculative, 1);
    if (spec == nullptr) {
        qCWarning(lcEngine) << "speculative decoding could not start, drafting off";
        return;
    }
    m_draftInit = std::move(draftInit);
    m_spec.reset(spec);
}

bool LlamaEngine::isLoaded() const
{
    return m_init != nullptr;
}

util::Result<Generation> LlamaEngine::generate(const GenerationRequest& request, const TextSink& sink)
{
    if (!isLoaded()) {
        return util::Result<Generation>::failure(
            QCoreApplication::translate("LlamaEngine", "No model is loaded."));
    }
    Run run(*this, request, sink);
    return run.execute();
}

std::vector<int> LlamaEngine::promptTokens(const GenerationRequest& request) const
{
    if (!isLoaded()) {
        return {};
    }
    const llama_vocab* vocab = llama_model_get_vocab(m_init->model());
    const common_chat_session session(m_templates.get(), vocab, chatInputs(request));
    return tokenize(vocab, session.prompt());
}

util::Result<bool> LlamaEngine::warm(const GenerationRequest& request)
{
    if (!isLoaded()) {
        return util::Result<bool>::failure(QCoreApplication::translate("LlamaEngine", "No model is loaded."));
    }
    const TextSink none; // Run keeps a reference
    Run run(*this, request, none);
    return run.prefill();
}

int LlamaEngine::countTokens(const QString& text) const
{
    if (!isLoaded()) {
        return 0;
    }
    const llama_vocab* vocab = llama_model_get_vocab(m_init->model());
    return static_cast<int>(common_tokenize(vocab, text.toStdString(), false, false).size());
}

void LlamaEngine::unload()
{
    m_spec.reset();
    m_draftInit.reset();
    m_templates.reset();
    m_defaults.reset();
    m_init.reset();
    m_cachedTokens.clear();
    m_draftCachedTokens.clear();
    m_config = {};
}

EngineConfig LlamaEngine::loadedConfig() const
{
    return isLoaded() ? m_config : EngineConfig{};
}

void LlamaEngine::cancel()
{
    m_cancel.store(true);
}

void LlamaEngine::resetCancel()
{
    m_cancel.store(false);
}

} // namespace paraphrase::engine
