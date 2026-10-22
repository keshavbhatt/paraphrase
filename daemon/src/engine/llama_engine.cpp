#include "llama_engine.h"

#include <QElapsedTimer>
#include <QJsonValue>
#include <QLoggingCategory>

#include <algorithm>
#include <chat.h>
#include <common.h>
#include <llama.h>
#include <mutex>
#include <sampling.h>
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

void initBackend()
{
    static std::once_flag once;
    std::call_once(once, [] {
        llama_log_set(llamaLog, nullptr);
        llama_backend_init();
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
    {
        for (const auto& stop : m_session.additional_stops()) {
            m_longestStop = std::max(m_longestStop, stop.size());
        }
    }

    util::Result<Generation> execute();

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
    void prepareVariants(const Tokens& prompt);
    bool step(Generation& out);
    void emit(int index, bool final);
    [[nodiscard]] size_t stopAt(const std::string& bytes) const;

    LlamaEngine& m_engine;
    const GenerationRequest& m_request;
    const TextSink& m_sink;
    llama_context* m_ctx;
    const llama_vocab* m_vocab;
    common_chat_session m_session;
    std::vector<Variant> m_variants;
    size_t m_longestStop = 0;
    QElapsedTimer m_clock;
};

util::Result<bool> LlamaEngine::Run::preparePrompt(Generation& out)
{
    const Tokens prompt = tokenize(m_vocab, m_session.prompt());
    if (static_cast<int>(prompt.size()) + m_request.maxTokens > m_engine.m_config.contextSize) {
        return util::Result<bool>::failure(
            u"The text is too long for this model (%1 tokens)."_s.arg(prompt.size()));
    }
    // Reuse the longest prefix already in the cache, as llama-server's prompt cache does; the
    // last prompt token is always decoded again for its logits.
    const auto& cached = m_engine.m_cachedTokens;
    size_t keep = 0;
    while (keep < cached.size() && keep < prompt.size() && cached[keep] == prompt[keep]) {
        ++keep;
    }
    keep = std::min(keep, prompt.size() - 1);
    llama_memory_t memory = llama_get_memory(m_ctx);
    if (!llama_memory_seq_rm(memory, 0, static_cast<llama_pos>(keep), -1)) {
        llama_memory_seq_rm(memory, 0, -1, -1);
        keep = 0;
    }
    for (int s = 1; s < m_engine.m_config.maxVariants; ++s) {
        llama_memory_seq_rm(memory, s, -1, -1);
    }
    out.promptTokens = static_cast<int>(prompt.size());
    out.cachedTokens = static_cast<int>(keep);
    if (!decodePrompt(m_ctx, prompt, keep, 0)) {
        m_engine.m_cachedTokens.clear();
        return util::Result<bool>::failure(u"The model could not read the text."_s);
    }
    m_engine.m_cachedTokens.assign(prompt.begin(), prompt.end());
    prepareVariants(prompt);
    return util::Result<bool>::success(true);
}

void LlamaEngine::Run::prepareVariants(const Tokens& prompt)
{
    common_params_sampling params = samplingFor(*m_engine.m_defaults, m_request.sampling);
    if (!m_request.grammar.isEmpty()) {
        params.grammar = common_grammar(COMMON_GRAMMAR_TYPE_USER, m_request.grammar.toStdString());
    }
    m_session.apply_sampling(params);
    const int n = std::clamp(m_request.variants, 1, m_engine.m_config.maxVariants);
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
        ++var.tokens;
        ++out.generatedTokens;
        if (llama_vocab_is_eog(m_vocab, tok)) {
            var.done = true;
            continue;
        }
        var.bytes += common_token_to_piece(m_ctx, tok, false);
        if (out.ttftSeconds == 0.0 && completeUtf8(var.bytes) > 0) {
            out.ttftSeconds = static_cast<double>(m_clock.nsecsElapsed()) / 1e9;
        }
        if (const size_t at = stopAt(var.bytes); at != std::string::npos) {
            var.bytes.resize(at);
            var.done = true;
            continue;
        }
        if (var.tokens >= m_request.maxTokens) {
            var.done = true;
            out.finishReason = u"length"_s;
            continue;
        }
        emit(static_cast<int>(v), false);
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

util::Result<Generation> LlamaEngine::Run::execute()
{
    m_clock.start();
    Generation out;
    out.finishReason = u"stop"_s;
    if (const auto ready = preparePrompt(out); !ready.ok()) {
        return util::Result<Generation>::failure(ready.error());
    }
    while (step(out)) {
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
                      << "prompt tokens," << out.cachedTokens << "cached," << out.totalSeconds << "s,"
                      << out.finishReason;
    return util::Result<Generation>::success(out);
}

void LlamaEngine::TemplatesDeleter::operator()(common_chat_templates* tmpls) const
{
    common_chat_templates_free(tmpls);
}

LlamaEngine::LlamaEngine() = default;
LlamaEngine::~LlamaEngine() = default;

util::Result<bool> LlamaEngine::load(const EngineConfig& config)
{
    initBackend();
    QElapsedTimer clock;
    clock.start();
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
    auto init = common_init_from_params(params);
    if (!init || init->model() == nullptr || init->context() == nullptr) {
        return util::Result<bool>::failure(u"The model could not be loaded from %1."_s.arg(config.modelPath));
    }
    m_templates.reset(common_chat_templates_init(init->model(), "").release());
    m_defaults = std::make_unique<common_params_sampling>(params.sampling);
    m_init = std::move(init);
    m_config = config;
    m_cachedTokens.clear();
    m_loadSeconds = static_cast<double>(clock.nsecsElapsed()) / 1e9;
    qCInfo(lcEngine) << "model loaded in" << m_loadSeconds << "s";
    return util::Result<bool>::success(true);
}

bool LlamaEngine::isLoaded() const
{
    return m_init != nullptr;
}

util::Result<Generation> LlamaEngine::generate(const GenerationRequest& request, const TextSink& sink)
{
    if (!isLoaded()) {
        return util::Result<Generation>::failure(u"No model is loaded."_s);
    }
    m_cancel.store(false);
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

void LlamaEngine::cancel()
{
    m_cancel.store(true);
}

} // namespace paraphrase::engine
