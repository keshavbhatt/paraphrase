#pragma once

#include "engine/engine.h"
#include "skills/skill.h"
#include "util/language_detector.h"

#include <QMutex>
#include <QObject>
#include <QString>

#include <functional>
#include <memory>

namespace paraphrase::api {

/// One request for the inference thread: the skill, its inputs and the model to run it on.
struct Job
{
    QString id;
    skills::Skill skill;
    skills::Values inputs;
    engine::EngineConfig model;
    QString modelId;
    bool warmOnly = false; ///< read the prompt ahead (ADR-005), generate nothing
};

/// Owns the engine on the inference thread (DOCS/CODING_STANDARDS.md, Threads): decoding
/// blocks for seconds, so the D-Bus thread hands it one job at a time. Loads the job's model
/// when another one is loaded, checks the skill's input limit, runs the skill with the
/// language check, and reports through queued signals. cancel() is the one call from another
/// thread.
class InferenceWorker : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(InferenceWorker)

public:
    using EngineFactory = std::function<std::unique_ptr<engine::Engine>()>;

    InferenceWorker(EngineFactory factory, const util::LanguageDetector& detector);
    ~InferenceWorker() override;

    /// Thread-safe: stops job `id` if it runs, or keeps it from starting if it is on its way.
    void cancel(const QString& id);

public Q_SLOTS:
    void run(const paraphrase::api::Job& job);
    void unload();

Q_SIGNALS:
    void token(const QString& id, int variant, const QString& text);
    void retrying(const QString& id, const QString& reason);
    /// `result` is the Done JSON (DOCS/api.md).
    void finished(const QString& id, const QString& result);
    void failed(const QString& id, const QString& code, const QString& message);
    void modelChanged(const QString& modelId); ///< empty when unloaded
    void idle();

private:
    [[nodiscard]] bool ensureModel(const Job& job);
    [[nodiscard]] int inputTokens(const Job& job) const;
    void warm(const Job& job);

    EngineFactory m_factory;
    const util::LanguageDetector& m_detector;
    std::unique_ptr<engine::Engine> m_engine; ///< loads and runs on the inference thread
    QString m_modelId;
    double m_loadSeconds = 0.0; ///< of the running job, when it loaded the model
    QMutex m_cancelLock;        ///< guards m_cancelled and the engine's cancel flag at job start
    QString m_cancelled;
};

} // namespace paraphrase::api

Q_DECLARE_METATYPE(paraphrase::api::Job)
