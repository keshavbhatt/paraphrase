#pragma once

#include "generator.h"
#include "skills/skill.h"
#include "util/language_detector.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

namespace paraphrase::engine {

/// What a skill run returns to a client (PLAN 5.1 `Done`).
struct SkillResult
{
    QString output;          ///< text, or {"replies": [...]} JSON for several variants
    QStringList texts;       ///< what the language check looked at: each reply, or the text
    bool retried = false;    ///< the language check failed once and the engine retried
    QString warning;         ///< set when the retry also failed: shown with the output
    QList<Generation> calls; ///< one, or two after a retry
};

/// Runs a skill the way the engine does (DOCS/skills.md, "Language check"): render, generate,
/// check the language of every output and retry once with a fresh seed and a line naming the
/// language; a second failure comes back with a warning, never hidden (ADR-006). A port of
/// eval/paraeval/engine.py, which the eval and the latency bench use.
class SkillRunner
{
public:
    static constexpr double kRetryTemperature = 0.3; ///< for skills that decode greedily
    static constexpr qint64 kRetrySeedOffset = 7919;

    SkillRunner(Generator& generator, const util::LanguageDetector& detector);

    [[nodiscard]] util::Result<SkillResult> run(const skills::Skill& skill, const skills::Values& inputs,
                                                const TextSink& sink = {});

    /// The line the retry adds after the prompt: the language the answer must be in.
    [[nodiscard]] QString retryHint(const skills::Values& inputs) const;
    /// Empty when every output is in the expected language, else why not.
    [[nodiscard]] QString languageProblem(const skills::Skill& skill, const skills::Values& inputs,
                                          const QStringList& texts) const;

private:
    Generator& m_generator;
    const util::LanguageDetector& m_detector;
};

/// The replies of {"replies": [...]} output, trimmed; nothing when the JSON has another shape.
[[nodiscard]] std::optional<QStringList> parseReplies(const QString& output);

} // namespace paraphrase::engine
