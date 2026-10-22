#include "inference_worker.h"

#include "engine/skill_runner.h"

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcInference, "paraphrase.inference")

namespace paraphrase::api {
namespace {

QJsonObject timings(const engine::Generation& g)
{
    return {
        {u"ttft_s"_s, g.ttftSeconds},
        {u"total_s"_s, g.totalSeconds},
        {u"prompt_tokens"_s, g.promptTokens},
        {u"cached_tokens"_s, g.cachedTokens},
        {u"generated_tokens"_s, g.generatedTokens},
        {u"draft_tokens"_s, g.draftTokens},
        {u"draft_accepted"_s, g.draftAccepted},
    };
}

/// The Done JSON (DOCS/api.md): the output as stored, the replies of a JSON skill, the
/// warning of a failed language check, and the timings of every model call.
QString resultJson(const Job& job, const engine::SkillResult& r, double loadSeconds)
{
    QJsonObject o{
        {u"request_id"_s, job.id},
        {u"skill"_s, job.skill.id},
        {u"model"_s, job.modelId},
        {u"output"_s, r.output},
        {u"retried"_s, r.retried},
        {u"warning"_s, r.warning},
        {u"finish"_s, r.calls.isEmpty() ? QString() : r.calls.last().finishReason},
        {u"load_s"_s, loadSeconds}, // this request loaded the model first; 0 when it was loaded
    };
    if (job.skill.outputType() == u"json"_s || job.skill.variants > 1) {
        o.insert(u"replies"_s, QJsonArray::fromStringList(r.texts));
    }
    QJsonArray calls;
    for (const auto& g : r.calls) {
        calls.append(timings(g));
    }
    o.insert(u"calls"_s, calls);
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

} // namespace

InferenceWorker::InferenceWorker(EngineFactory factory, const util::LanguageDetector& detector)
    : m_factory(std::move(factory))
    , m_detector(detector)
    , m_engines(makeEngines(m_factory))
    , m_engine(m_engines.front().get())
{}

std::array<std::unique_ptr<engine::Engine>, InferenceWorker::kSlots>
InferenceWorker::makeEngines(const EngineFactory& factory)
{
    std::array<std::unique_ptr<engine::Engine>, kSlots> engines;
    for (auto& engine : engines) {
        engine = factory();
    }
    return engines;
}

InferenceWorker::~InferenceWorker() = default;

void InferenceWorker::cancel(const QString& id)
{
    const QMutexLocker lock(&m_cancelLock);
    m_cancelled = id;
    for (const auto& engine : m_engines) {
        engine->cancel(); // only atomic state: safe from this thread
    }
}

bool InferenceWorker::cancelled(const QString& id) const
{
    const QMutexLocker lock(&m_cancelLock);
    return m_cancelled == id;
}

bool InferenceWorker::holds(const engine::Engine& engine, const engine::EngineConfig& model)
{
    return engine.isLoaded() && engine.loadedConfig().modelPath == model.modelPath &&
           engine.loadedConfig().draftModelPath == model.draftModelPath;
}

std::size_t InferenceWorker::slotFor(const Job& job)
{
    if (job.residentModels < 2) {
        // Only one model fits: keep the first slot, free the other before loading.
        for (std::size_t i = 1; i < kSlots; ++i) {
            if (m_engines.at(i)->isLoaded()) {
                m_engines.at(i)->unload();
                m_slotModels.at(i).clear();
            }
        }
        return 0;
    }
    std::size_t best = 0;
    for (std::size_t i = 0; i < kSlots; ++i) {
        if (!m_engines.at(i)->isLoaded()) {
            return i;
        }
        if (m_lastUsed.at(i) < m_lastUsed.at(best)) {
            best = i;
        }
    }
    return best;
}

bool InferenceWorker::ensureModel(const Job& job)
{
    for (std::size_t i = 0; i < kSlots; ++i) {
        if (holds(*m_engines.at(i), job.model) && (job.residentModels >= 2 || i == 0)) {
            m_engine = m_engines.at(i).get();
            m_lastUsed.at(i) = ++m_uses;
            if (m_modelId != job.modelId) {
                m_modelId = job.modelId;
                Q_EMIT modelChanged(m_modelId);
            }
            return true;
        }
    }
    const std::size_t slot = slotFor(job);
    m_engine = m_engines.at(slot).get();
    m_lastUsed.at(slot) = ++m_uses;
    qCInfo(lcInference) << "loading" << job.modelId << "in slot" << slot;
    QElapsedTimer clock;
    clock.start();
    const auto loaded = m_engine->load(job.model);
    m_loadSeconds = static_cast<double>(clock.elapsed()) / 1000.0;
    if (!loaded.ok()) {
        m_modelId.clear();
        Q_EMIT modelChanged({});
        Q_EMIT failed(job.id, u"model"_s, loaded.error());
        return false;
    }
    m_slotModels.at(slot) = job.modelId;
    m_modelId = job.modelId;
    Q_EMIT modelChanged(m_modelId);
    return true;
}

int InferenceWorker::inputTokens(const Job& job) const
{
    return m_engine->countTokens(job.inputs.value(u"text"_s)) +
           m_engine->countTokens(job.inputs.value(u"context"_s));
}

void InferenceWorker::run(const Job& job)
{
    m_loadSeconds = 0.0;
    {
        const QMutexLocker lock(&m_cancelLock);
        if (m_cancelled == job.id) {
            if (!job.warmOnly) {
                Q_EMIT failed(job.id, u"cancelled"_s, tr("Cancelled."));
            }
            Q_EMIT idle();
            return;
        }
        for (const auto& engine : m_engines) {
            engine->resetCancel(); // a cancel from here on is for this job
        }
    }
    if (!ensureModel(job)) {
        Q_EMIT idle();
        return;
    }
    if (cancelled(job.id)) { // cancelled while the model loaded
        if (!job.warmOnly) {
            Q_EMIT failed(job.id, u"cancelled"_s, tr("Cancelled."));
        }
        Q_EMIT idle();
        return;
    }
    const int tokens = inputTokens(job);
    if (job.skill.maxInputTokens > 0 && tokens > job.skill.maxInputTokens) {
        if (!job.warmOnly) {
            Q_EMIT failed(job.id, u"too-long"_s,
                          tr("The text is too long for %1: %2 tokens, at most %3. Select less text.")
                              .arg(job.skill.title)
                              .arg(tokens)
                              .arg(job.skill.maxInputTokens));
        }
        Q_EMIT idle();
        return;
    }
    if (job.warmOnly) {
        warm(job);
        Q_EMIT idle();
        return;
    }

    // Logged by id and size only: never the text (CLAUDE.md).
    qCInfo(lcInference) << job.id << job.skill.id << "on" << job.modelId << tokens << "input tokens";
    engine::SkillRunner runner(*m_engine, m_detector);
    const QString id = job.id;
    const auto result = runner.run(
        job.skill, job.inputs,
        [this, &id](int variant, const QString& piece) { Q_EMIT token(id, variant, piece); },
        [this, &id](const QString& why) { Q_EMIT retrying(id, why); });
    report(job, result);
    Q_EMIT idle();
}

void InferenceWorker::report(const Job& job, const util::Result<engine::SkillResult>& result)
{
    if (!result.ok()) {
        Q_EMIT failed(job.id, u"engine"_s, result.error());
    } else if (!result.value().calls.isEmpty() &&
               result.value().calls.last().finishReason == u"cancelled"_s) {
        Q_EMIT failed(job.id, u"cancelled"_s, tr("Cancelled."));
    } else if (result.value().incomplete) {
        Q_EMIT failed(job.id, u"incomplete"_s,
                      tr("The answer left out part of the text, so it is not shown. Try a shorter "
                         "selection."));
    } else {
        Q_EMIT finished(job.id, resultJson(job, result.value(), m_loadSeconds));
    }
}

void InferenceWorker::warm(const Job& job)
{
    const auto prompt = job.skill.render(job.inputs);
    const engine::GenerationRequest request{
        .system = prompt.system,
        .user = prompt.user,
        .sampling = job.skill.sampling,
        .grammar = job.skill.grammar,
        .maxTokens = job.skill.maxOutputTokens,
        .variants = job.skill.variants,
        .draft = job.skill.speculative == u"draft"_s,
    };
    if (const auto warmed = m_engine->warm(request); !warmed.ok()) {
        qCWarning(lcInference) << job.id << "read-ahead failed:" << warmed.error();
    }
}

void InferenceWorker::unload()
{
    bool any = false;
    for (std::size_t i = 0; i < kSlots; ++i) {
        if (m_engines.at(i)->isLoaded()) {
            m_engines.at(i)->unload();
            m_slotModels.at(i).clear();
            any = true;
        }
    }
    if (any) {
        qCInfo(lcInference) << "models unloaded";
        m_modelId.clear();
        Q_EMIT modelChanged({});
    }
}

} // namespace paraphrase::api
