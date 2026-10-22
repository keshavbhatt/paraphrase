#pragma once

#include "engine/engine.h"
#include "engine/skill_runner.h"
#include "skills/skill.h"
#include "util/language_detector.h"

#include <QMutex>
#include <QObject>
#include <QString>

#include <array>
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
    /// How many models may stay loaded at once: 2 when the computer's memory holds the fast and
    /// the quality model together (switching then costs no load), else 1.
    int residentModels = 1;
};

/// Owns the engines on the inference thread (DOCS/CODING_STANDARDS.md, Threads): decoding
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
    [[nodiscard]] bool cancelled(const QString& id) const;
    /// Emits finished() or failed() for a run's result.
    void report(const Job& job, const util::Result<engine::SkillResult>& result);
    [[nodiscard]] int inputTokens(const Job& job) const;
    void warm(const Job& job);
    /// The slot to load `job`'s model into: a free one when two may stay loaded, else the one
    /// used longest ago (the others are unloaded when only one may stay).
    [[nodiscard]] std::size_t slotFor(const Job& job);
    [[nodiscard]] static bool holds(const engine::Engine& engine, const engine::EngineConfig& model);

    EngineFactory m_factory;
    const util::LanguageDetector& m_detector;
    static constexpr std::size_t kSlots = 2;
    /// Created up front and never replaced: cancel() reaches them from another thread.
    std::array<std::unique_ptr<engine::Engine>, kSlots> m_engines;
    std::array<QString, kSlots> m_slotModels; ///< model id loaded in each slot
    std::array<qint64, kSlots> m_lastUsed{};  ///< a use counter, for the slot used longest ago
    qint64 m_uses = 0;
    engine::Engine* m_engine = nullptr; ///< the slot of the running (or last) job
    QString m_modelId;
    double m_loadSeconds = 0.0;  ///< of the running job, when it loaded the model
    mutable QMutex m_cancelLock; ///< guards m_cancelled and the engine's cancel flag at job start
    QString m_cancelled;
};

} // namespace paraphrase::api

Q_DECLARE_METATYPE(paraphrase::api::Job)
