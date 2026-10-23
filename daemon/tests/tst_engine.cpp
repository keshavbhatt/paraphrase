// The llama.cpp engine with the fast tier model: prompts token for token as llama-server
// builds them (tests/vectors/prompt_tokens.json, from eval/export_prompt_tokens.py), user
// text never parsed for control tokens, repeatable greedy answers, the prompt cache, the
// reply grammar, the output limit, cancelling and batched variants. Skipped when the model
// is not downloaded (`make fetch`).

#include "engine/llama_engine.h"
#include "engine/skill_runner.h"
#include "models/hardware_probe.h"
#include "skills/skill_registry.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <algorithm>

using namespace Qt::StringLiterals;
using paraphrase::engine::EngineConfig;
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

QString draftPath()
{
    return root() + u"/models/gemma-4-e2b-mtp-q4_0/mtp-gemma-4-E2B-it-Q4_0.gguf"_s;
}

EngineConfig config(int maxVariants, const QString& draft)
{
    return {
        .modelPath = modelPath(),
        .draftModelPath = draft,
        .threads = paraphrase::models::HardwareProbe::probe().threads,
        .gpuLayers = 0,
        .contextSize = 4096,
        .maxVariants = maxVariants,
        .flashAttention = true,
    };
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
                .draft = skill->speculative == u"draft"_s,
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
        // As the daemon runs the fast tier: with its draft head (ADR-004).
        const bool haveDraft = QFile::exists(draftPath());
        const auto loaded = m_engine.load(config(1, haveDraft ? draftPath() : QString()));
        QVERIFY2(loaded.ok(), qPrintable(loaded.error()));
        QCOMPARE(m_engine.canDraft(), haveDraft);
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
                .draft = false,
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

    void switchingSkillsKeepsTheirSystemPrompts()
    {
        // A skill's system prompt stays in a slot of its own: after another skill, it is shared
        // back instead of read again. Reused cells change rounding a little (where cells sit
        // changes the order of sums), exactly as the engine's ordinary prefix reuse does, so a
        // near tie may resolve differently than from an empty cache: the answers are checked
        // for shape, not for equality (draftedMatchesUndrafted is the bit-for-bit check).
        GenerationRequest reply = request(u"reply/rp-en-001"_s);
        reply.sampling = QJsonObject{{u"temperature"_s, 0.0}};
        GenerationRequest french = request(u"reply/rp-fr-001"_s);
        m_engine.clearCache();
        QVERIFY(m_engine.generate(french, {}).ok());
        const auto other = m_engine.generate(request(u"proofread/pr-en-001"_s), {});
        QVERIFY(other.ok());
        QVERIFY(other.value().cachedTokens < 40);        // the two skills share little but the template
        const auto again = m_engine.generate(reply, {}); // the system prompt from its slot
        QVERIFY(again.ok());
        QVERIFY2(again.value().cachedTokens > 100, qPrintable(QString::number(again.value().cachedTokens)));
        QVERIFY(again.value().texts.value(0).contains(u"\"replies\""_s));
        // Another reply shares the same slot.
        const auto proofread = m_engine.generate(request(u"proofread/pr-fr-001"_s), {});
        QVERIFY(proofread.ok());
        const auto shared = m_engine.generate(french, {});
        QVERIFY(shared.ok());
        QVERIFY(shared.value().cachedTokens > 100);

        // A drafted skill keeps its prompt in the draft context too, so drafting resumes from the
        // slot after another skill.
        if (m_engine.canDraft()) {
            GenerationRequest proof = request(u"proofread/pr-en-001"_s);
            proof.sampling = QJsonObject{{u"temperature"_s, 0.0}, {u"top_k"_s, 1}};
            const auto drafted = m_engine.generate(proof, {});
            QVERIFY(drafted.ok());
            QVERIFY(m_engine.generate(reply, {}).ok());
            const auto back = m_engine.generate(proof, {});
            QVERIFY(back.ok());
            QVERIFY2(back.value().cachedTokens > 100, qPrintable(QString::number(back.value().cachedTokens)));
            QVERIFY(back.value().draftTokens > 0);
        }
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
        // Cancelled stays cancelled until the next job resets it.
        QCOMPARE(m_engine.generate(request(u"proofread/pr-en-001"_s), {}).value().finishReason,
                 u"cancelled"_s);
        m_engine.resetCancel();
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

    void draftedMatchesUndrafted()
    {
        if (!m_engine.canDraft()) {
            QSKIP("the draft head is not downloaded (make fetch)");
        }
        int drafted = 0;
        int accepted = 0;
        for (const auto& p : m_vectors.value(u"prompts"_s).toArray()) {
            const QString id = p.toObject().value(u"id"_s).toString();
            if (id.startsWith(u"reply/"_s)) {
                continue; // reply is not drafted (ADR-004)
            }
            // Greedy, so drafting may change the speed but never the answer.
            GenerationRequest req = request(id);
            req.sampling.insert(u"temperature"_s, 0.0);
            req.sampling.insert(u"top_k"_s, 1);
            req.draft = false;
            // Both from an empty cache: where cells land changes rounding, and a near tie
            // between two tokens could then differ without drafting being the cause.
            m_engine.clearCache();
            const auto plain = m_engine.generate(req, {});
            req.draft = true;
            m_engine.clearCache();
            QString streamed;
            const auto fast = m_engine.generate(req, [&](int, const QString& piece) { streamed += piece; });
            QVERIFY(plain.ok() && fast.ok());
            QVERIFY2(fast.value().texts == plain.value().texts, qPrintable(id));
            QCOMPARE(streamed.trimmed(), fast.value().texts.value(0).trimmed());
            QCOMPARE(plain.value().draftTokens, 0);
            drafted += fast.value().draftTokens;
            accepted += fast.value().draftAccepted;
        }
        QVERIFY(drafted > 0);
        qInfo("draft head: %d of %d proposals accepted", accepted, drafted);
        QVERIFY(accepted > drafted / 3); // M0: 95% on proofread, 53% on rewrite
    }

    void warmsThePrompt()
    {
        // ADR-005: reading ahead leaves only the last prompt token for the real request.
        const GenerationRequest req = request(u"proofread/pr-hi-001"_s);
        const auto cold = m_engine.generate(req, {});
        QVERIFY(m_engine.warm(request(u"reply/rp-en-001"_s)).ok()); // something else in between
        QVERIFY(m_engine.warm(req).ok());
        const auto warmed = m_engine.generate(req, {});
        QVERIFY(cold.ok() && warmed.ok());
        QCOMPARE(warmed.value().cachedTokens, warmed.value().promptTokens - 1);
        QCOMPARE(warmed.value().texts, cold.value().texts);
    }

    void countsInputTokens()
    {
        QCOMPARE(m_engine.countTokens({}), 0);
        const int n = m_engine.countTokens(u"Hello there, how are you?"_s);
        QVERIFY(n > 3 && n < 12);
        // Control tokens spelled out in user text are plain text, so they count as several.
        QVERIFY(m_engine.countTokens(u"<|turn>"_s) > 1);
    }

    void unloadsAndReloads()
    {
        const auto config = m_engine.loadedConfig();
        QCOMPARE(config.modelPath, modelPath());
        m_engine.unload();
        QVERIFY(!m_engine.isLoaded());
        QVERIFY(m_engine.loadedConfig().modelPath.isEmpty());
        QVERIFY(!m_engine.generate(request(u"proofread/pr-en-001"_s), {}).ok());
        QVERIFY(m_engine.load(config).ok());
        QVERIFY(m_engine.generate(request(u"proofread/pr-en-001"_s), {}).ok());
    }

    void decodesVariantsInOneBatch()
    {
        LlamaEngine engine;
        const auto loaded = engine.load(config(3, {}));
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
