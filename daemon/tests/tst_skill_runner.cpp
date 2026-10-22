// The skill runner behaves as the Python reference (eval/paraeval/engine.py): one call when
// the output keeps its language, one retry naming the language when it does not, and a
// warning, never a hidden output, when the retry fails too. A scripted generator stands in
// for the model.

#include "engine/skill_runner.h"
#include "skills/request_inputs.h"
#include "skills/skill_registry.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using namespace Qt::StringLiterals;
using paraphrase::engine::Generation;
using paraphrase::engine::GenerationRequest;
using paraphrase::engine::Generator;
using paraphrase::engine::parseReplies;
using paraphrase::engine::SkillRunner;
using paraphrase::engine::TextSink;
using paraphrase::skills::buildInputs;
using paraphrase::skills::RequestFields;
using paraphrase::skills::Skill;
using paraphrase::skills::SkillRegistry;
using paraphrase::util::LanguageData;
using paraphrase::util::LanguageDetector;
using paraphrase::util::Result;

namespace {

const QString kEnglish =
    u"Their going to the market tomorrow morning to buy some fresh vegetables for dinner."_s;
const QString kHindi = u"मैं कल सुबह बाज़ार जाऊँगा और रात के खाने के लिए कुछ ताज़ी सब्ज़ियाँ खरीदूँगा।"_s;
const QString kFrench = u"Je vais au marché demain matin pour acheter des légumes frais pour le dîner."_s;

/// Answers each call with the next scripted text and keeps the requests it was given.
class ScriptedGenerator final : public Generator
{
public:
    explicit ScriptedGenerator(QList<QStringList> answers)
        : m_answers(std::move(answers))
    {}

    Result<Generation> generate(const GenerationRequest& request, const TextSink& sink) override
    {
        requests.append(request);
        if (m_answers.isEmpty()) {
            return Result<Generation>::failure(u"the model stopped"_s);
        }
        Generation g;
        g.texts = m_answers.takeFirst();
        g.finishReason = u"stop"_s;
        for (int v = 0; v < g.texts.size(); ++v) {
            if (sink) {
                sink(v, g.texts.at(v));
            }
        }
        return Result<Generation>::success(g);
    }

    QList<GenerationRequest> requests;

private:
    QList<QStringList> m_answers;
};

RequestFields textField(const QString& text, const QString& targetLang = {})
{
    return {.text = text, .context = {}, .targetLang = targetLang};
}

RequestFields contextField(const QString& context)
{
    return {.text = {}, .context = context, .targetLang = {}};
}

SkillRegistry shippedSkills()
{
    auto loaded = SkillRegistry::load(QStringLiteral(PARAPHRASE_REPO_ROOT) + u"/skills"_s);
    if (!loaded.ok()) {
        qFatal("%s", qPrintable(loaded.error()));
    }
    return std::move(loaded).value();
}

} // namespace

class TestSkillRunner : public QObject
{
    Q_OBJECT

private:
    SkillRegistry m_skills = shippedSkills();
    LanguageDetector m_detector{LanguageData::builtIn()};

    [[nodiscard]] const Skill& skill(const QString& id) const
    {
        const Skill* s = m_skills.find(id);
        if (s == nullptr) {
            qFatal("no skill %s", qPrintable(id));
        }
        return *s;
    }

    [[nodiscard]] paraphrase::skills::Values inputs(const QString& id, const RequestFields& fields) const
    {
        return buildInputs(skill(id).baseId, fields, m_detector);
    }

private Q_SLOTS:
    void keepsLanguageInOneCall()
    {
        ScriptedGenerator gen({{kEnglish}});
        SkillRunner runner(gen, m_detector);
        const auto& proofread = skill(u"proofread"_s);
        const auto in = inputs(u"proofread"_s, textField(kEnglish));

        QStringList streamed;
        const auto result =
            runner.run(proofread, in, [&](int, const QString& piece) { streamed.append(piece); });
        QVERIFY(result.ok());
        QCOMPARE(result.value().output, kEnglish);
        QVERIFY(!result.value().retried);
        QVERIFY(result.value().warning.isEmpty());
        QCOMPARE(result.value().calls.size(), 1);
        QCOMPARE(streamed, QStringList{kEnglish});

        const auto prompt = proofread.render(in);
        QCOMPARE(gen.requests.size(), 1);
        QCOMPARE(gen.requests.at(0).system, prompt.system);
        QCOMPARE(gen.requests.at(0).user, prompt.user);
        QCOMPARE(gen.requests.at(0).sampling, proofread.sampling);
        QCOMPARE(gen.requests.at(0).maxTokens, proofread.maxOutputTokens);
        QCOMPARE(gen.requests.at(0).variants, 1);
    }

    void retriesOnceNamingTheLanguage()
    {
        ScriptedGenerator gen({{kEnglish}, {kHindi}});
        SkillRunner runner(gen, m_detector);
        const auto& proofread = skill(u"proofread"_s);
        const auto in = inputs(u"proofread"_s, textField(kHindi));

        const auto result = runner.run(proofread, in);
        QVERIFY(result.ok());
        QCOMPARE(result.value().output, kHindi);
        QVERIFY(result.value().retried);
        QVERIFY(result.value().warning.isEmpty());
        QCOMPARE(result.value().calls.size(), 2);

        QCOMPARE(gen.requests.size(), 2);
        const auto& retry = gen.requests.at(1);
        QCOMPARE(retry.user,
                 proofread.render(in).user +
                     u"\n\nImportant: write your answer in Hindi, the language of the original."_s);
        // Greedy proofread gets a little temperature and a new seed, else it repeats itself.
        QCOMPARE(retry.sampling.value(u"temperature"_s).toDouble(), SkillRunner::kRetryTemperature);
        QCOMPARE(retry.sampling.value(u"seed"_s).toInteger(), SkillRunner::kRetrySeedOffset);
        QCOMPARE(retry.sampling.value(u"top_k"_s), proofread.sampling.value(u"top_k"_s));
    }

    void warnsWhenTheRetryFailsToo()
    {
        ScriptedGenerator gen({{kEnglish}, {kEnglish}});
        SkillRunner runner(gen, m_detector);
        const auto result = runner.run(skill(u"proofread"_s), inputs(u"proofread"_s, textField(kHindi)));
        QVERIFY(result.ok());
        QCOMPARE(result.value().output, kEnglish); // shown, never hidden (ADR-006)
        QVERIFY(result.value().retried);
        QCOMPARE(result.value().warning, u"language: Hindi answered outside Devanagari"_s);
        QCOMPARE(gen.requests.size(), 2);
    }

    void checksAnExplicitTargetLanguage()
    {
        const auto& rewrite = skill(u"rewrite.professional"_s);
        const auto in = inputs(u"rewrite.professional"_s, textField(kEnglish, u"fr"_s));

        ScriptedGenerator gen({{kEnglish}, {kEnglish}});
        SkillRunner runner(gen, m_detector);
        const auto result = runner.run(rewrite, in);
        QVERIFY(result.ok());
        QCOMPARE(result.value().warning, u"language: asked for fr, got en"_s);
        QVERIFY(gen.requests.at(1).user.endsWith(
            u"Important: write your answer in French, as the user asked."_s));

        ScriptedGenerator good({{kFrench}});
        SkillRunner goodRunner(good, m_detector);
        const auto ok = goodRunner.run(rewrite, in);
        QVERIFY(ok.ok());
        QVERIFY(ok.value().warning.isEmpty());
        QCOMPARE(good.requests.size(), 1);
    }

    void checksEachReply()
    {
        const auto& reply = skill(u"reply"_s);
        const auto in = inputs(u"reply"_s, contextField(kHindi));
        const QString mixed =
            QString::fromUtf8(QJsonDocument(QJsonObject{{u"replies"_s, QJsonArray{kHindi, kEnglish, kHindi}}})
                                  .toJson(QJsonDocument::Compact));
        const QString hindi =
            QString::fromUtf8(QJsonDocument(QJsonObject{{u"replies"_s, QJsonArray{kHindi, kHindi, kHindi}}})
                                  .toJson(QJsonDocument::Compact));

        ScriptedGenerator gen({{mixed}, {hindi}});
        SkillRunner runner(gen, m_detector);
        const auto result = runner.run(reply, in);
        QVERIFY(result.ok());
        QCOMPARE(result.value().output, hindi);
        QCOMPARE(result.value().texts, (QStringList{kHindi, kHindi, kHindi}));
        QVERIFY(result.value().retried);
        QCOMPARE(gen.requests.at(0).grammar, reply.grammar);
        // reply samples (temperature 0.7, seed null): only the seed changes on a retry.
        QCOMPARE(gen.requests.at(1).sampling.value(u"temperature"_s).toDouble(), 0.7);
        QCOMPARE(gen.requests.at(1).sampling.value(u"seed"_s).toInteger(), SkillRunner::kRetrySeedOffset);
    }

    void checksInvalidJsonAsOneText()
    {
        ScriptedGenerator gen({{kHindi}});
        SkillRunner runner(gen, m_detector);
        const auto result = runner.run(skill(u"reply"_s), inputs(u"reply"_s, contextField(kHindi)));
        QVERIFY(result.ok());
        QCOMPARE(result.value().texts, QStringList{kHindi});
        QVERIFY(!result.value().retried);
    }

    void joinsVariantsAsReplies()
    {
        Skill variants = skill(u"reply"_s);
        variants.variants = 3;
        variants.output.insert(u"type"_s, u"text"_s);
        ScriptedGenerator gen({{u" Sure. "_s, u"Maybe later?\n"_s, u"No thanks."_s}});
        SkillRunner runner(gen, m_detector);
        const auto result = runner.run(variants, inputs(u"reply"_s, contextField(kEnglish)));
        QVERIFY(result.ok());
        QCOMPARE(result.value().output, uR"({"replies":["Sure.","Maybe later?","No thanks."]})"_s);
        QCOMPARE(gen.requests.at(0).variants, 3);
    }

    void passesGeneratorErrorsOn()
    {
        ScriptedGenerator gen({});
        SkillRunner runner(gen, m_detector);
        const auto result = runner.run(skill(u"proofread"_s), inputs(u"proofread"_s, textField(kEnglish)));
        QVERIFY(!result.ok());
        QCOMPARE(result.error(), u"the model stopped"_s);
    }

    void retryHintForAnUnclearLanguage()
    {
        ScriptedGenerator gen({});
        SkillRunner runner(gen, m_detector);
        QCOMPARE(runner.retryHint({{u"lang"_s, u"und"_s}}),
                 u"Important: write your answer in the same language and script as the original."_s);
    }

    void parsesReplies()
    {
        QCOMPARE(parseReplies(uR"({"replies": [" a ", "b"]})"_s).value_or(QStringList{u"none"_s}),
                 (QStringList{u"a"_s, u"b"_s}));
        QCOMPARE(parseReplies(uR"({"replies": []})"_s).value_or(QStringList{u"none"_s}), QStringList{});
        QVERIFY(!parseReplies(uR"({"replies": [1]})"_s).has_value());
        QVERIFY(!parseReplies(uR"({"other": []})"_s).has_value());
        QVERIFY(!parseReplies(uR"(["a"])"_s).has_value());
        QVERIFY(!parseReplies(u"not json"_s).has_value());
    }
};

QTEST_GUILESS_MAIN(TestSkillRunner)
#include "tst_skill_runner.moc"
