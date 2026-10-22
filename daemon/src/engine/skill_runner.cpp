#include "skill_runner.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Qt::StringLiterals;

namespace paraphrase::engine {
namespace {

QString expectedLang(const skills::Values& inputs)
{
    const QString target = inputs.value(u"target_lang"_s);
    return target.isEmpty() ? inputs.value(u"lang"_s) : target;
}

/// A greedy skill gets a little temperature (else the retry repeats itself) and every retry a
/// fixed new seed, as the Python reference.
QJsonObject retrySampling(QJsonObject sampling)
{
    if (sampling.value(u"temperature"_s).toDouble() == 0.0) {
        sampling.insert(u"temperature"_s, SkillRunner::kRetryTemperature);
    }
    const qint64 seed = sampling.value(u"seed"_s).isDouble() ? sampling.value(u"seed"_s).toInteger() : 0;
    sampling.insert(u"seed"_s, seed + SkillRunner::kRetrySeedOffset);
    return sampling;
}

} // namespace

std::optional<QStringList> parseReplies(const QString& output)
{
    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return std::nullopt;
    }
    const QJsonValue replies = doc.object().value(u"replies"_s);
    if (!replies.isArray()) {
        return std::nullopt;
    }
    QStringList out;
    for (const auto& r : replies.toArray()) {
        if (!r.isString()) {
            return std::nullopt;
        }
        out.append(r.toString().trimmed());
    }
    return out;
}

SkillRunner::SkillRunner(Generator& generator, const util::LanguageDetector& detector)
    : m_generator(generator)
    , m_detector(detector)
{}

QString SkillRunner::retryHint(const skills::Values& inputs) const
{
    const QString want = expectedLang(inputs);
    const QString name = m_detector.plainName(want);
    if (want != inputs.value(u"lang"_s)) {
        return u"Important: write your answer in %1, as the user asked."_s.arg(name);
    }
    if (name.isEmpty()) {
        return u"Important: write your answer in the same language and script as the original."_s;
    }
    return u"Important: write your answer in %1, the language of the original."_s.arg(name);
}

QString SkillRunner::languageProblem(const skills::Skill& skill, const skills::Values& inputs,
                                     const QStringList& texts) const
{
    if (!skill.keepsLanguage()) {
        return {};
    }
    const QString want = expectedLang(inputs);
    const QString lang = inputs.value(u"lang"_s);
    const QString context = inputs.value(u"context"_s);
    const QString source = context.isEmpty() ? inputs.value(u"text"_s) : context;
    for (const QString& t : texts) {
        if (want != lang) {
            // An explicit target language: the output itself must read as that language.
            if (const QString got = m_detector.detect(t); got != want) {
                return u"asked for %1, got %2"_s.arg(want, got);
            }
            continue;
        }
        if (const auto check = m_detector.check(want, source, t); !check.ok) {
            return check.reason;
        }
    }
    return {};
}

util::Result<SkillResult> SkillRunner::run(const skills::Skill& skill, const skills::Values& inputs,
                                           const TextSink& sink)
{
    const auto prompt = skill.render(inputs);
    GenerationRequest request{
        .system = prompt.system,
        .user = prompt.user,
        .sampling = skill.sampling,
        .grammar = skill.grammar,
        .maxTokens = skill.maxOutputTokens,
        .variants = skill.variants,
        .draft = skill.speculative == u"draft"_s,
    };
    SkillResult result;
    QString problem;
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (attempt == 1) {
            request.user = prompt.user + u"\n\n"_s + retryHint(inputs);
            request.sampling = retrySampling(skill.sampling);
        }
        const auto generated = m_generator.generate(request, sink);
        if (!generated.ok()) {
            return util::Result<SkillResult>::failure(generated.error());
        }
        const Generation& g = generated.value();
        result.calls.append(g);
        if (skill.variants > 1) {
            QJsonArray replies;
            result.texts.clear();
            for (const QString& t : g.texts) {
                replies.append(t.trimmed());
                result.texts.append(t.trimmed());
            }
            result.output = QString::fromUtf8(
                QJsonDocument(QJsonObject{{u"replies"_s, replies}}).toJson(QJsonDocument::Compact));
        } else {
            result.output = g.texts.value(0);
            const auto replies = skill.outputType() == u"json"_s ? parseReplies(result.output) : std::nullopt;
            result.texts = replies.value_or(QStringList{result.output});
        }
        problem = languageProblem(skill, inputs, result.texts);
        result.retried = attempt > 0;
        if (problem.isEmpty()) {
            return util::Result<SkillResult>::success(std::move(result));
        }
    }
    result.warning = u"language: "_s + problem;
    return util::Result<SkillResult>::success(std::move(result));
}

} // namespace paraphrase::engine
