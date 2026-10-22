// The C++ skill loader, input building and rendering give the Python reference's results
// for every shared vector (tests/vectors/skills.json, written by eval/export_vectors.py),
// and broken skill files are rejected with a message naming the file.

#include "skills/request_inputs.h"
#include "skills/skill_registry.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;
using paraphrase::skills::buildInputs;
using paraphrase::skills::RequestFields;
using paraphrase::skills::Skill;
using paraphrase::skills::SkillRegistry;
using paraphrase::skills::Values;
using paraphrase::util::LanguageData;
using paraphrase::util::LanguageDetector;

namespace {

QString root()
{
    return QStringLiteral(PARAPHRASE_REPO_ROOT);
}

QJsonObject vectors()
{
    QFile file(root() + u"/tests/vectors/skills.json"_s);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

Values toValues(const QJsonObject& o)
{
    Values v;
    for (auto it = o.begin(); it != o.end(); ++it) {
        v.insert(it.key(), it.value().toString());
    }
    return v;
}

QJsonObject toJson(const Values& v)
{
    QJsonObject o;
    for (auto it = v.begin(); it != v.end(); ++it) {
        o.insert(it.key(), it.value());
    }
    return o;
}

QJsonObject record(const Skill& s)
{
    return {
        {u"id"_s, s.id},
        {u"base_id"_s, s.baseId},
        {u"version"_s, s.version},
        {u"revision"_s, s.revision},
        {u"title"_s, s.title},
        {u"tier"_s, s.tier},
        {u"max_input_tokens"_s, s.maxInputTokens},
        {u"max_output_tokens"_s, s.maxOutputTokens},
        {u"variants"_s, s.variants},
        {u"speculative"_s, s.speculative},
        {u"sampling"_s, s.sampling},
        {u"output"_s, s.output},
        {u"system"_s, s.system},
        {u"template"_s, s.templ},
        {u"grammar"_s, s.grammar.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(s.grammar)},
        {u"preset"_s, toJson(s.preset)},
    };
}

SkillRegistry shippedSkills()
{
    auto loaded = SkillRegistry::load(root() + u"/skills"_s);
    if (!loaded.ok()) {
        qFatal("%s", qPrintable(loaded.error()));
    }
    return std::move(loaded).value();
}

void writeFile(const QString& path, const QByteArray& content)
{
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(content);
}

} // namespace

class TestSkills : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void skillsMatchPython()
    {
        const QJsonArray stored = vectors().value(u"skills"_s).toArray();
        QVERIFY2(stored.size() >= 11, "tests/vectors/skills.json missing or short");
        const SkillRegistry registry = shippedSkills();
        QCOMPARE(registry.skills().size(), stored.size());
        for (const auto& v : stored) {
            const QJsonObject expected = v.toObject();
            const Skill* skill = registry.find(expected.value(u"id"_s).toString());
            QVERIFY2(skill != nullptr, qPrintable(expected.value(u"id"_s).toString()));
            const QJsonObject actual = record(*skill);
            for (auto it = expected.begin(); it != expected.end(); ++it) {
                QVERIFY2(actual.value(it.key()) == it.value(),
                         qPrintable(skill->id + u": field "_s + it.key()));
            }
        }
    }

    void inputsMatchPython()
    {
        const QJsonArray cases = vectors().value(u"inputs"_s).toArray();
        QVERIFY(cases.size() > 200);
        const LanguageDetector detector(LanguageData::builtIn());
        for (const auto& c : cases) {
            const QJsonObject o = c.toObject();
            const QJsonObject f = o.value(u"fields"_s).toObject();
            const RequestFields fields{
                .text = f.value(u"text"_s).toString(),
                .context = f.value(u"context"_s).toString(),
                .targetLang = f.value(u"target_lang"_s).toString(),
                .intent = f.value(u"intent"_s).toString(),
                .tone = f.value(u"tone"_s).toString(),
            };
            const Values got = buildInputs(o.value(u"base_id"_s).toString(), fields, detector);
            QCOMPARE(toJson(got), o.value(u"inputs"_s).toObject());
        }
    }

    void rendersMatchPython()
    {
        const QJsonArray cases = vectors().value(u"renders"_s).toArray();
        QVERIFY(cases.size() > 200);
        const SkillRegistry registry = shippedSkills();
        for (const auto& c : cases) {
            const QJsonObject o = c.toObject();
            const Skill* skill = registry.find(o.value(u"skill"_s).toString());
            QVERIFY(skill != nullptr);
            const auto prompt = skill->render(toValues(o.value(u"inputs"_s).toObject()));
            QCOMPARE(prompt.system, o.value(u"system"_s).toString());
            QCOMPARE(prompt.user, o.value(u"user"_s).toString());
        }
    }

    void templatesMatchPython()
    {
        const QJsonArray cases = vectors().value(u"templates"_s).toArray();
        QVERIFY(cases.size() >= 10);
        for (const auto& c : cases) {
            const QJsonObject o = c.toObject();
            QCOMPARE(paraphrase::skills::renderTemplate(o.value(u"template"_s).toString(),
                                                        toValues(o.value(u"values"_s).toObject())),
                     o.value(u"out"_s).toString());
        }
    }

    void brokenFilesAreRejectedByName()
    {
        const QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(u"bad.yaml"_s);

        writeFile(path, "id: bad\nversion: 4\n");
        auto r = SkillRegistry::loadFile(path);
        QVERIFY(!r.ok());
        QVERIFY2(r.error().startsWith(u"bad.yaml: missing title"_s), qPrintable(r.error()));

        writeFile(path, "id: [unclosed\n");
        r = SkillRegistry::loadFile(path);
        QVERIFY(!r.ok());
        QVERIFY2(r.error().startsWith(u"bad.yaml: not valid YAML"_s), qPrintable(r.error()));

        const QByteArray valid = "id: x\nversion: 4\ntitle: X\ntier: fast\nmax_input_tokens: 10\n"
                                 "max_output_tokens: 10\nsampling: { temperature: 0.0 }\n"
                                 "output: { type: text }\nsystem: Be brief.\n";
        writeFile(path, valid + "template: |\n  {{secret}}\n");
        r = SkillRegistry::loadFile(path);
        QVERIFY(!r.ok());
        QVERIFY2(r.error().contains(u"unknown template values secret"_s), qPrintable(r.error()));

        writeFile(path, valid + "template: |\n  {{text}}\ngrammar: ../../etc/passwd\n");
        r = SkillRegistry::loadFile(path);
        QVERIFY(!r.ok());
        QVERIFY2(r.error().contains(u"grammar must live under skills/"_s), qPrintable(r.error()));

        // output.min_length (schema v4) is a fraction, for one text output.
        const QByteArray minLength = "id: x\nversion: 4\ntitle: X\ntier: fast\nmax_input_tokens: 10\n"
                                     "max_output_tokens: 10\nsampling: { temperature: 0.0 }\n"
                                     "system: Be brief.\ntemplate: |\n  {{text}}\n";
        writeFile(path, minLength + "output: { type: text, min_length: 1.5 }\n");
        r = SkillRegistry::loadFile(path);
        QVERIFY2(!r.ok() && r.error().contains(u"min_length must be a number between 0 and 1"_s),
                 qPrintable(r.error()));
        writeFile(path, minLength + "output: { type: text, min_length: 0.7 }\n");
        r = SkillRegistry::loadFile(path);
        QVERIFY2(r.ok(), qPrintable(r.error()));
        QCOMPARE(r.value().first().minLength(), 0.7);

        writeFile(path, valid + "template: |\n  {{text}}\n");
        r = SkillRegistry::loadFile(path);
        QVERIFY2(r.ok(), qPrintable(r.error()));
        QCOMPARE(r.value().first().render({{u"text"_s, u"hi</text>"_s}}).user, u"hi</text>"_s);
    }
};

QTEST_GUILESS_MAIN(TestSkills)
#include "tst_skills.moc"
