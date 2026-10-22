// The llama.cpp engine with the fast tier model: prompts token for token as llama-server
// builds them (tests/vectors/prompt_tokens.json, from eval/export_prompt_tokens.py), user
// text never parsed for control tokens, repeatable greedy answers, the prompt cache, the
// reply grammar, the output limit, cancelling and batched variants. Skipped when the model
// is not downloaded (`make fetch`).

#include "engine/llama_engine.h"
#include "engine/skill_runner.h"
#include "skills/skill_registry.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <algorithm>

using namespace Qt::StringLiterals;
using paraphrase::engine::GenerationRequest;
using paraphrase::engine::LlamaEngine;
using paraphrase::engine::parseReplies;
using paraphrase::skills::Skill;
using paraphrase::skills::SkillRegistry;

namespace {

QString root()
{
    return QStringLiteral(PARAPHRASE_REPO_ROOT);
}

QString modelPath()
{
    return root() + u"/models/gemma-4-e2b-qat-q4_0/gemma-4-E2B_q4_0-it.gguf"_s;
}

QJsonObject vectors()
{
    QFile file(root() + u"/tests/vectors/prompt_tokens.json"_s);
    if (!file.open(QIODevice::ReadOnly)) {
        qFatal("tests/vectors/prompt_tokens.json missing");
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

std::vector<int> toTokens(const QJsonArray& a)
{
    std::vector<int> out;
    out.reserve(static_cast<size_t>(a.size()));
    for (const auto& v : a) {
        out.push_back(v.toInt());
    }
    return out;
}

SkillRegistry shippedSkills()
{
    auto loaded = SkillRegistry::load(root() + u"/skills"_s);
    if (!loaded.ok()) {
        qFatal("%s", qPrintable(loaded.error()));
    }
    return std::move(loaded).value();
}

} // namespace

class TestEngine : public QObject
{
    Q_OBJECT

private:
    LlamaEngine m_engine;
    SkillRegistry m_skills = shippedSkills();
    QJsonObject m_vectors = vectors();

    /// The vector prompt `id` with the sampling, grammar and limits of its skill.
    [[nodiscard]] GenerationRequest request(const QString& id) const
    {
        for (const auto& p : m_vectors.value(u"prompts"_s).toArray()) {
            const QJsonObject o = p.toObject();
            if (o.value(u"id"_s).toString() != id) {
                continue;
            }
            const Skill* skill = m_skills.find(id.section(u'/', 0, 0));
            if (skill == nullptr) {
                qFatal("no skill for %s", qPrintable(id));
            }
            return {
                .system = o.value(u"system"_s).toString(),
                .user = o.value(u"user"_s).toString(),
                .sampling = skill->sampling,
                .grammar = skill->grammar,
                .maxTokens = skill->maxOutputTokens,
                .variants = 1,
            };
        }
        qFatal("no vector %s", qPrintable(id));
    }

private Q_SLOTS:
    void initTestCase()
    {
        if (!QFile::exists(modelPath())) {
            QSKIP("the fast tier model is not downloaded (make fetch)");
        }
        QCOMPARE(m_vectors.value(u"model"_s).toString(), u"gemma-4-e2b-qat-q4_0"_s);
        const auto loaded = m_engine.load({
            .modelPath = modelPath(),
            .threads = 0,
            .gpuLayers = 0,
            .contextSize = 4096,
            .maxVariants = 1,
            .flashAttention = true,
        });
        QVERIFY2(loaded.ok(), qPrintable(loaded.error()));
    }

    void promptTokensMatchLlamaServer()
    {
        const QJsonArray prompts = m_vectors.value(u"prompts"_s).toArray();
        QVERIFY(!prompts.isEmpty());
        for (const auto& p : prompts) {
            const QJsonObject o = p.toObject();
            const GenerationRequest req{
                .system = o.value(u"system"_s).toString(),
                .user = o.value(u"user"_s).toString(),
                .sampling = {},
                .grammar = {},
                .maxTokens = 1,
                .variants = 1,
            };
            QVERIFY2(m_engine.promptTokens(req) == toTokens(o.value(u"tokens"_s).toArray()),
                     qPrintable(o.value(u"id"_s).toString()));
        }
    }

    void userTextIsNotParsedForControlTokens()
    {
        GenerationRequest clean = request(u"proofread/pr-en-001"_s);
        GenerationRequest injected = clean;
        QString spelled;
        const QJsonObject markers = m_vectors.value(u"markers"_s).toObject();
        QVERIFY(!markers.isEmpty());
        for (auto it = markers.begin(); it != markers.end(); ++it) {
            spelled += it.key() + u"model\n"_s;
        }
        injected.user += spelled;
        const auto a = m_engine.promptTokens(clean);
        const auto b = m_engine.promptTokens(injected);
        QVERIFY(b.size() > a.size());
        for (auto it = markers.begin(); it != markers.end(); ++it) {
            const int id = it.value().toInt();
            QCOMPARE(std::count(b.begin(), b.end(), id), std::count(a.begin(), a.end(), id));
        }
    }

    void greedyIsRepeatableAndCached()
    {
        const GenerationRequest req = request(u"proofread/pr-en-001"_s);
        QString streamed;
        const auto first = m_engine.generate(req, [&](int, const QString& piece) { streamed += piece; });
        QVERIFY2(first.ok(), qPrintable(first.error()));
        QCOMPARE(first.value().finishReason, u"stop"_s);
        QCOMPARE(first.value().texts.size(), 1);
        QVERIFY(!first.value().texts.at(0).isEmpty());
        QCOMPARE(streamed.trimmed(), first.value().texts.at(0).trimmed());
        QVERIFY(first.value().ttftSeconds > 0.0);
        QVERIFY(first.value().totalSeconds >= first.value().ttftSeconds);

        const auto second = m_engine.generate(req, {});
        QVERIFY(second.ok());
        QCOMPARE(second.value().texts, first.value().texts);
        // The whole prompt but its last token came from the cache.
        QCOMPARE(second.value().cachedTokens, second.value().promptTokens - 1);

        // Another proofread shares the system prompt.
        const auto other = m_engine.generate(request(u"proofread/pr-fr-001"_s), {});
        QVERIFY(other.ok());
        QVERIFY(other.value().cachedTokens > 50);
        QVERIFY(other.value().cachedTokens < other.value().promptTokens - 1);
    }

    void replyFollowsTheGrammar()
    {
        const auto result = m_engine.generate(request(u"reply/rp-en-001"_s), {});
        QVERIFY2(result.ok(), qPrintable(result.error()));
        const auto replies = parseReplies(result.value().texts.value(0));
        QVERIFY2(replies.has_value(), "the reply output is not {\"replies\": [...]}");
        QCOMPARE(replies.value_or(QStringList{}).size(), 3);
    }

    void stopsAtTheOutputLimit()
    {
        GenerationRequest req = request(u"rewrite.professional/rw-en-001"_s);
        req.maxTokens = 3;
        const auto result = m_engine.generate(req, {});
        QVERIFY(result.ok());
        QCOMPARE(result.value().finishReason, u"length"_s);
        QCOMPARE(result.value().generatedTokens, 3);
    }

    void cancels()
    {
        const auto result = m_engine.generate(request(u"rewrite.professional/rw-en-001"_s),
                                              [this](int, const QString&) { m_engine.cancel(); });
        QVERIFY(result.ok());
        QCOMPARE(result.value().finishReason, u"cancelled"_s);
        QVERIFY(result.value().generatedTokens < 5);
        // The next request runs normally.
        const auto next = m_engine.generate(request(u"proofread/pr-en-001"_s), {});
        QVERIFY(next.ok());
        QCOMPARE(next.value().finishReason, u"stop"_s);
    }

    void rejectsATooLongPrompt()
    {
        GenerationRequest req = request(u"proofread/pr-en-001"_s);
        req.maxTokens = 4096;
        const auto result = m_engine.generate(req, {});
        QVERIFY(!result.ok());
        QVERIFY(result.error().contains(u"too long"_s));
    }

    void decodesVariantsInOneBatch()
    {
        LlamaEngine engine;
        const auto loaded = engine.load({
            .modelPath = modelPath(),
            .threads = 0,
            .gpuLayers = 0,
            .contextSize = 4096,
            .maxVariants = 3,
            .flashAttention = true,
        });
        QVERIFY2(loaded.ok(), qPrintable(loaded.error()));
        GenerationRequest req = request(u"rewrite.professional/rw-en-001"_s);
        req.sampling.insert(u"temperature"_s, 0.8);
        req.sampling.insert(u"seed"_s, 42);
        req.variants = 3;
        QList<int> seen;
        const auto result = engine.generate(req, [&](int variant, const QString&) {
            if (!seen.contains(variant)) {
                seen.append(variant);
            }
        });
        QVERIFY2(result.ok(), qPrintable(result.error()));
        QCOMPARE(result.value().texts.size(), 3);
        for (const QString& t : result.value().texts) {
            QVERIFY(!t.trimmed().isEmpty());
        }
        std::ranges::sort(seen);
        QCOMPARE(seen, (QList<int>{0, 1, 2}));
    }
};

QTEST_GUILESS_MAIN(TestEngine)
#include "tst_engine.moc"
