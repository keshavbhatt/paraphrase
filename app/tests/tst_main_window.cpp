// The standalone main window (ADR-013) against a real daemon service with a fake engine, on a
// private socket: the actions come from the skills; proofread shows the word diff and undoes
// one change at a time; reply shows three replies and passes the draft; a tone runs from the
// Tone menu; a missing model and a stopped request are shown in words. With
// PARAPHRASE_SCREENSHOTS=<dir> it also saves the window in light and dark for review.

#include "fake_daemon.h"
#include "ui/main_window.h"

#include <QAbstractButton>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBrowser>
#include <QToolButton>

using namespace Qt::StringLiterals;
using namespace paraphrase::testing;
using paraphrase::app::MainWindow;

namespace {

constexpr int kTimeoutMs = 10'000;

} // namespace

class TestMainWindow : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::shared_ptr<FakeState> m_state;
    std::unique_ptr<ServerThread> m_server;

    void startDaemon(const QStringList& installed)
    {
        m_dir = std::make_unique<QTemporaryDir>();
        m_state = std::make_shared<FakeState>();
        const auto catalog = paraphrase::models::ModelCatalog::parse(tinyCatalog()).value();
        QVERIFY(install(catalog, m_dir->path() + u"/models"_s, installed));
        auto skills =
            paraphrase::skills::SkillRegistry::load(QStringLiteral(PARAPHRASE_REPO_ROOT) + u"/skills"_s);
        QVERIFY(skills.ok());
        paraphrase::models::HardwareInfo hw;
        hw.totalMemory = qint64{32} * 1024 * 1024 * 1024;
        hw.threads = 4;
        const QString address = u"unix:path="_s + m_dir->path() + u"/daemon.sock"_s;
        m_server = std::make_unique<ServerThread>(
            ServiceSetup{
                .skills = std::move(skills).value(),
                .catalog = catalog,
                .modelsRoot = m_dir->path() + u"/models"_s,
                .hardware = hw,
                .engineFactory = [state = m_state] { return std::make_unique<FakeEngine>(state); },
                .urlResolver = {},
                .idleUnloadMs = 60'000,
                .idleExitMs = 60'000,
            },
            address);
        m_server->start();
        QTRY_VERIFY_WITH_TIMEOUT(m_server->ready.load(), kTimeoutMs);
        qputenv("PARAPHRASE_DAEMON_ADDRESS", address.toUtf8());
    }

    void setAnswers(const QStringList& answers)
    {
        const QMutexLocker lock(&m_state->lock);
        m_state->answers = answers;
    }

    static void runAndWait(MainWindow& w)
    {
        const QSignalSpy finished(&w, &MainWindow::finished);
        w.run();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, kTimeoutMs);
    }

    static QString outputText(MainWindow& w)
    {
        return w.findChild<QTextBrowser*>(u"output"_s)->toPlainText();
    }

private Q_SLOTS:
    void cleanup()
    {
        qunsetenv("PARAPHRASE_DAEMON_ADDRESS");
        if (m_server) {
            m_server->quit();
            m_server->wait();
            m_server.reset();
        }
    }

    void buildsTheActionsFromTheSkills()
    {
        startDaemon({u"fast-model"_s});
        MainWindow w;
        w.start();
        QVERIFY(w.findChild<QAbstractButton*>(u"action-proofread"_s) != nullptr);
        QVERIFY(w.findChild<QAbstractButton*>(u"action-reply"_s) != nullptr);
        const auto* tone = w.findChild<QToolButton*>(u"action-rewrite"_s);
        QVERIFY(tone != nullptr && tone->menu() != nullptr);
        QStringList tones;
        for (const QAction* a : tone->menu()->actions()) {
            tones.append(a->objectName());
        }
        QVERIFY(tones.contains(u"rewrite.professional"_s) && tones.contains(u"rewrite.casual"_s));
        QCOMPARE(w.currentSkill(), u"proofread"_s);
        QVERIFY(w.findChild<QLabel*>(u"statusText"_s)->text().contains(u"fast-model"_s));
    }

    void proofreadShowsTheDiffAndUndoesOneChange()
    {
        startDaemon({u"fast-model"_s});
        MainWindow w;
        w.start();
        w.findChild<QPlainTextEdit*>(u"text"_s)->setPlainText(u"Whisk the egg's with the milk"_s);
        setAnswers({u"Whisk the eggs with the milk."_s});
        w.selectSkill(u"proofread"_s);
        runAndWait(w);
        QCOMPARE(w.resultText(), u"Whisk the eggs with the milk."_s);
        QVERIFY(w.findChild<QLabel*>(u"outInfo"_s)->text().contains(u"2 changes"_s));
        // Undo the first change only: "egg's" comes back, the full stop stays.
        auto undos = w.findChildren<QPushButton*>(QRegularExpression(u"^undo\\d+$"_s));
        QCOMPARE(undos.size(), 2);
        std::ranges::sort(undos, {}, [](const QPushButton* b) { return b->objectName().mid(4).toInt(); });
        undos.front()->click();
        QCOMPARE(w.resultText(), u"Whisk the egg's with the milk."_s);
        w.copyResult();
        QCOMPARE(QApplication::clipboard()->text(), u"Whisk the egg's with the milk."_s);
    }

    void replyShowsThreeRepliesAndPassesTheDraft()
    {
        startDaemon({u"fast-model"_s});
        MainWindow w;
        w.start();
        w.selectSkill(u"reply"_s);
        w.findChild<QPlainTextEdit*>(u"message"_s)->setPlainText(u"Are you free for dinner on Friday?"_s);
        w.findChild<QPlainTextEdit*>(u"draft"_s)->setPlainText(u"Yes"_s);
        setAnswers(
            {uR"({"replies": ["Yes, Friday works!", "Yes, what time?", "Yes, can I bring dessert?"]})"_s});
        runAndWait(w);
        QVERIFY(w.findChild<QPushButton*>(u"reply2"_s)->text().contains(u"Yes, what time?"_s));
        QCOMPARE(w.findChild<QLineEdit*>(u"replyEdit"_s)->text(), u"Yes, Friday works!"_s);
        w.findChild<QPushButton*>(u"reply3"_s)->click();
        QCOMPARE(w.resultText(), u"Yes, can I bring dessert?"_s);
        w.copyResult();
        QCOMPARE(QApplication::clipboard()->text(), u"Yes, can I bring dessert?"_s);
    }

    void runsAToneFromTheMenu()
    {
        startDaemon({u"fast-model"_s, u"quality-model"_s});
        MainWindow w;
        w.start();
        w.findChild<QPlainTextEdit*>(u"text"_s)->setPlainText(u"send me the file now"_s);
        setAnswers({u"Could you send me the file, please?"_s});
        const QSignalSpy finished(&w, &MainWindow::finished);
        const auto* tone = w.findChild<QToolButton*>(u"action-rewrite"_s);
        for (QAction* a : tone->menu()->actions()) {
            if (a->objectName() == u"rewrite.polite"_s) {
                a->trigger();
            }
        }
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, kTimeoutMs);
        QCOMPARE(w.currentSkill(), u"rewrite.polite"_s);
        QCOMPARE(w.resultText(), u"Could you send me the file, please?"_s);
        QVERIFY(tone->text().contains(u"Polite"_s));
    }

    void saysWhenNoModelIsInstalled()
    {
        startDaemon({});
        MainWindow w;
        w.start();
        QVERIFY(w.findChild<QLabel*>(u"statusText"_s)->text().contains(u"No model"_s));
        w.findChild<QPlainTextEdit*>(u"text"_s)->setPlainText(u"teh cat"_s);
        runAndWait(w);
        QVERIFY(outputText(w).contains(u"No model is installed"_s));
    }

    void stopsARunningRequest()
    {
        startDaemon({u"fast-model"_s});
        {
            const QMutexLocker lock(&m_state->lock);
            m_state->defaultAnswer = QString(u"word "_s).repeated(200).trimmed();
            m_state->pieceDelayMs = 20;
        }
        MainWindow w;
        w.start();
        w.findChild<QPlainTextEdit*>(u"text"_s)->setPlainText(u"Their going home."_s);
        const QSignalSpy finished(&w, &MainWindow::finished);
        w.run();
        QTRY_VERIFY_WITH_TIMEOUT(!outputText(w).isEmpty(), kTimeoutMs); // it streams
        w.cancel();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, kTimeoutMs);
        QVERIFY(!w.busy());
        QVERIFY(w.findChild<QLabel*>(u"outInfo"_s)->text().contains(u"stopped"_s));
    }

    void savesScreenshots()
    {
        const QString dir = qEnvironmentVariable("PARAPHRASE_SCREENSHOTS");
        if (dir.isEmpty()) {
            QSKIP("set PARAPHRASE_SCREENSHOTS=<dir> to save the window in light and dark");
        }
        QDir().mkpath(dir);
        for (const char* theme : {"light", "dark"}) {
            qputenv("PARAPHRASE_THEME", theme);
            startDaemon({u"fast-model"_s});
            {
                MainWindow w;
                w.resize(1180, 720);
                w.show();
                w.start();
                w.findChild<QPlainTextEdit*>(u"text"_s)->setPlainText(
                    u"Pancakes (serves 4)\n1. Mix the flour, sugar and a pinch of salt in a large bowl.\n"
                    "2. Whisk the egg's with the milk and add it slowly to the dry ingredients\n"
                    "3. Let the batter rest for 10 minute.\n4. Cook on a medium heat until bubbles appears."_s);
                setAnswers({
                    u"Pancakes (serves 4)\n1. Mix the flour, sugar and a pinch of salt in a large bowl.\n"
                    "2. Whisk the eggs with the milk and add it slowly to the dry ingredients.\n"
                    "3. Let the batter rest for 10 minutes.\n4. Cook on a medium heat until bubbles appear."_s,
                });
                runAndWait(w);
                QVERIFY(
                    w.grab().save(dir + u"/main-window-proofread-%1.png"_s.arg(QLatin1StringView(theme))));
                w.selectSkill(u"reply"_s);
                w.findChild<QPlainTextEdit*>(u"message"_s)
                    ->setPlainText(u"बेटा, रविवार को दादी का जन्मदिन है। सब लोग दोपहर 12 बजे तक घर आ जाना।"_s);
                setAnswers(
                    {uR"({"replies": ["ठीक है, मैं 12 बजे तक आ जाऊँगा।", "क्या मैं कुछ लेकर आऊँ?", "ज़रूर, दादी को मेरी ओर से बधाई!"]})"_s});
                runAndWait(w);
                QVERIFY(w.grab().save(dir + u"/main-window-reply-%1.png"_s.arg(QLatin1StringView(theme))));
            }
            cleanup();
        }
        qunsetenv("PARAPHRASE_THEME");
    }
};

QTEST_MAIN(TestMainWindow)
#include "tst_main_window.moc"
