// `paraphrase` as a user runs it (M1 acceptance: proofread, reply and rewrite from the CLI):
// the real binary starts the real daemon privately (ADR-010) and runs the fast tier model.
// Without the model only the usage checks run.

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTest>

using namespace Qt::StringLiterals;

namespace {

constexpr int kTimeoutMs = 180'000;

QString root()
{
    return QStringLiteral(PARAPHRASE_REPO_ROOT);
}

struct Run
{
    int code = -1;
    QString out;
    QString err;
};

/// Runs the CLI with `args`, feeding `input` on standard input (a pipe, so nothing streams).
Run paraphrase(const QStringList& args, const QByteArray& input = {})
{
    QProcess p;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(u"PARAPHRASE_PRIVATE"_s, u"1"_s); // never touch a session bus from a test
    env.insert(u"PARAPHRASE_DAEMON"_s, QStringLiteral(PARAPHRASE_DAEMON_BIN));
    env.insert(u"PARAPHRASE_MODELS_DIR"_s, root() + u"/models"_s);
    p.setProcessEnvironment(env);
    p.start(QStringLiteral(PARAPHRASE_CLI), args);
    p.write(input);
    p.closeWriteChannel();
    if (!p.waitForFinished(kTimeoutMs)) {
        p.kill();
        return {};
    }
    return {
        .code = p.exitCode(),
        .out = QString::fromUtf8(p.readAllStandardOutput()),
        .err = QString::fromUtf8(p.readAllStandardError()),
    };
}

bool haveModel()
{
    return QFile::exists(root() + u"/models/gemma-4-e2b-qat-q4_0/gemma-4-E2B_q4_0-it.gguf.sha256"_s);
}

} // namespace

class TestCli : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void showsUsage()
    {
        const Run help = paraphrase({u"--help"_s});
        QCOMPARE(help.code, 0);
        QVERIFY(help.out.contains(u"proofread"_s) && help.out.contains(u"models"_s));
        QCOMPARE(paraphrase({u"frobnicate"_s}).code, 2);
    }

    void listsSkillsAndModels()
    {
        const Run skills = paraphrase({u"skills"_s});
        QCOMPARE(skills.code, 0);
        QVERIFY2(skills.out.contains(u"rewrite.casual"_s), qPrintable(skills.err));
        const Run models = paraphrase({u"models"_s});
        QCOMPARE(models.code, 0);
        QVERIFY(models.out.contains(u"gemma-4-e2b-qat-q4_0"_s));
        QCOMPARE(paraphrase({u"models"_s, u"download"_s}).code, 2);
    }

    void refusesBadInput()
    {
        QCOMPARE(paraphrase({u"proofread"_s}, "  \n").code, 2); // no text
        QCOMPARE(paraphrase({u"rewrite"_s, u"--tone"_s, u"grumpy"_s, u"hi"_s}).code, 2);
        QCOMPARE(paraphrase({u"proofread"_s, u"--to"_s, u"xx"_s, u"hi"_s}).code, 2);
    }

    void proofreads()
    {
        if (!haveModel()) {
            QSKIP("the fast tier model is not downloaded (make fetch)");
        }
        const Run r = paraphrase({u"proofread"_s}, "Their going to the market tomorow, dont be late.");
        QVERIFY2(r.code == 0, qPrintable(r.err));
        QVERIFY2(r.out.contains(u"market"_s) && r.out.contains(u"tomorrow"_s), qPrintable(r.out));
        QVERIFY(r.out.endsWith(u'\n'));
    }

    void replies()
    {
        if (!haveModel()) {
            QSKIP("the fast tier model is not downloaded (make fetch)");
        }
        const Run r = paraphrase({u"reply"_s, u"Are you free for dinner on Friday?"_s});
        QVERIFY2(r.code == 0, qPrintable(r.err));
        QCOMPARE(r.out.trimmed().split(u'\n').size(), 3); // one suggestion per line
    }

    void rewritesWithJson()
    {
        if (!haveModel()) {
            QSKIP("the fast tier model is not downloaded (make fetch)");
        }
        const Run r =
            paraphrase({u"rewrite"_s, u"--tone"_s, u"polite"_s, u"--json"_s, u"send me the file now"_s});
        QVERIFY2(r.code == 0, qPrintable(r.err));
        const QJsonObject done = QJsonDocument::fromJson(r.out.toUtf8()).object();
        QCOMPARE(done.value(u"skill"_s).toString(), u"rewrite.polite"_s);
        QVERIFY(!done.value(u"output"_s).toString().isEmpty());
        QVERIFY(!done.value(u"model"_s).toString().isEmpty());
    }
    void runsABatch()
    {
        if (!haveModel()) {
            QSKIP("the fast tier model is not downloaded (make fetch)");
        }
        const QByteArray requests = R"({"skill": "proofread", "input": {"text": "i has a apple"}}
{"skill": "nonsense", "input": {"text": "x"}}
{"skill": "reply", "input": {"context": "Lunch at noon?"}}
)";
        const Run r = paraphrase({u"batch"_s, u"--model"_s, u"gemma-4-e2b-qat-q4_0"_s}, requests);
        QVERIFY2(r.code == 0, qPrintable(r.err));
        const QStringList lines = r.out.trimmed().split(u'\n');
        QCOMPARE(lines.size(), 3);
        const QJsonObject first = QJsonDocument::fromJson(lines.at(0).toUtf8()).object();
        QCOMPARE(first.value(u"skill"_s).toString(), u"proofread"_s);
        QCOMPARE(first.value(u"model"_s).toString(), u"gemma-4-e2b-qat-q4_0"_s);
        QVERIFY(first.value(u"load_s"_s).toDouble() > 0.0); // the first request loads the model
        QVERIFY(first.value(u"client_ttft_s"_s).toDouble() > 0.0);
        QVERIFY(first.value(u"client_total_s"_s).toDouble() >= first.value(u"client_ttft_s"_s).toDouble());
        QCOMPARE(QJsonDocument::fromJson(lines.at(1).toUtf8()).object().value(u"error"_s).toString(),
                 u"InvalidInput"_s);
        const QJsonObject third = QJsonDocument::fromJson(lines.at(2).toUtf8()).object();
        QCOMPARE(third.value(u"replies"_s).toArray().size(), 3);
        QCOMPARE(third.value(u"load_s"_s).toDouble(), 0.0); // already loaded
    }
};

QTEST_GUILESS_MAIN(TestCli)
#include "tst_cli.moc"
