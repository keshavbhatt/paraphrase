// The standalone main window (ADR-013) against a real daemon service with a fake engine, on a
// private socket: the actions come from the skills; proofread shows the word diff and undoes
// one change at a time; reply shows three replies and passes the draft; a tone runs from the
// Tone menu; a missing model and a stopped request are shown in words. With
// PARAPHRASE_SCREENSHOTS=<dir> it also saves the window in light and dark for review.

#include "app_daemon.h"
#include "daemon_client.h"
#include "ui/common/chip_button.h"
#include "ui/main/main_window.h"
#include "ui/parts/input_pages.h"
#include "ui/parts/tone_picker.h"

#include <QAbstractButton>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMutexLocker>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBrowser>

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
    AppDaemon m_daemon;

    void startDaemon(const QStringList& installed) { QVERIFY(m_daemon.start(installed)); }
    void setAnswers(const QStringList& answers) const { m_daemon.setAnswers(answers); }

    static void runAndWait(MainWindow& w)
    {
        const QSignalSpy finished(&w, &MainWindow::finished);
        w.run();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, kTimeoutMs);
    }

    /// The window once Qt has laid out what the last answer added (rows are shown queued).
    static QPixmap grabNow(QWidget& w)
    {
        QApplication::processEvents();
        return w.grab();
    }

    static QString outputText(MainWindow& w)
    {
        return w.findChild<QTextBrowser*>(u"output"_s)->toPlainText();
    }

    QTemporaryDir m_settingsDir; ///< the tests' own QSettings, never the user's

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_settingsDir.isValid());
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_settingsDir.path());
    }
    void init() { QSettings().clear(); } // a tone or kind one test picks never reaches the next
    void cleanup() { m_daemon.stop(); }

    void buildsTheActionsFromTheSkills()
    {
        startDaemon({u"fast-model"_s});
        MainWindow w;
        w.start();
        QVERIFY(w.findChild<QAbstractButton*>(u"action-proofread"_s) != nullptr);
        QVERIFY(w.findChild<QAbstractButton*>(u"action-reply"_s) != nullptr);
        const auto* tone = w.findChild<paraphrase::app::ChipButton*>(u"action-rewrite"_s);
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
        // The change rows show under the text, with room for both (main-window.html, note 4).
        w.resize(1180, 760);
        w.show();
        QApplication::processEvents();
        const auto* changes = w.findChild<QWidget*>(u"changes"_s);
        QVERIFY(changes->isVisible());
        QVERIFY(changes->height() >= 2 * 28);
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
        QCOMPARE(w.findChild<paraphrase::app::ChipButton*>(u"reply2"_s)->label(), u"Yes, what time?"_s);
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
        const auto* tone = w.findChild<paraphrase::app::ChipButton*>(u"action-rewrite"_s);
        const int width = tone->sizeHint().width();
        for (QAction* a : tone->menu()->actions()) {
            if (a->objectName() == u"rewrite.polite"_s) {
                a->trigger();
            }
        }
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, kTimeoutMs);
        QCOMPARE(w.currentSkill(), u"rewrite.polite"_s);
        QCOMPARE(w.resultText(), u"Could you send me the file, please?"_s);
        // A rewrite that drops a fact says so (popup-rewrite.html, note 4).
        w.findChild<QPlainTextEdit*>(u"text"_s)->setPlainText(u"tell Priya the report is due Friday"_s);
        setAnswers({u"Could you let her know the report is due soon?"_s});
        runAndWait(w);
        QVERIFY(w.findChild<QLabel*>(u"outInfo"_s)->text().contains(u"check: Priya, Friday are missing"_s));
        // The chip keeps its label and size, so the row never re-wraps under the pointer.
        QCOMPARE(tone->label(), u"Tone"_s);
        QCOMPARE(tone->sizeHint().width(), width);
        QCOMPARE(tone->toolTip(), u"Tone: Polite"_s);
        QVERIFY(w.findChild<QLabel*>(u"outInfo"_s)->text().startsWith(u"· polite"_s));
    }

    void writesAFullReplyAndKeepsItsVersions()
    {
        startDaemon({u"fast-model"_s, u"quality-model"_s});
        MainWindow w;
        w.start();
        w.selectSkill(u"reply"_s);
        auto* full = w.findChild<QPushButton*>(u"fullReply"_s);
        if (full == nullptr || full->parentWidget()->isHidden()) {
            QSKIP("no full reply skill (skills/draft.yaml) in this tree");
        }
        full->click();
        QVERIFY(w.currentSkill().startsWith(u"draft."_s));
        w.findChild<QPlainTextEdit*>(u"fullMessage"_s)->setPlainText(u"Can we meet on Thursday at 3 pm?"_s);
        w.findChild<QPlainTextEdit*>(u"intent"_s)->setPlainText(u"Thursday is bad, offer Friday 11 am"_s);
        setAnswers({
            u"Hi Dan, Thursday does not work for me. Would Friday at 11 am suit you? Best, Priya"_s,
            u"Hi Dan, could we do Friday at 11 am instead? Thanks, Priya"_s,
        });
        runAndWait(w);
        QVERIFY(w.resultText().contains(u"Friday at 11 am"_s));
        {
            const QMutexLocker lock(&m_daemon.state->lock);
            QVERIFY(m_daemon.state->prompts.last().contains(
                u"offer Friday 11 am"_s)); // the intent reached the model
        }
        // "Another version" keeps the first one behind ‹ ›.
        const QSignalSpy finished(&w, &MainWindow::finished);
        w.run();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, kTimeoutMs);
        QCOMPARE(w.findChild<QLabel*>(u"versionInfo"_s)->text(), u"Version 2 of 2"_s);
        QVERIFY(w.resultText().startsWith(u"Hi Dan, could we"_s));
    }

    void translatesIntoTheChosenLanguage()
    {
        startDaemon({u"fast-model"_s, u"quality-model"_s});
        MainWindow w;
        w.start();
        const auto* translate = w.findChild<paraphrase::app::ChipButton*>(u"action-translate"_s);
        if (translate == nullptr) {
            QSKIP("no translate skill in this tree");
        }
        w.findChild<QPlainTextEdit*>(u"text"_s)->setPlainText(u"The meeting moved to Friday."_s);
        setAnswers({u"La réunion a été déplacée à vendredi."_s});
        Q_EMIT translate->menu()->aboutToShow(); // the menu fills itself when it opens
        auto* french = translate->menu()->findChild<QAction*>(u"lang-fr"_s);
        QVERIFY(french != nullptr);
        QVERIFY(translate->menu()->findChild<QAction*>(u"lang-en"_s) == nullptr); // the text's own language
        const QSignalSpy finished(&w, &MainWindow::finished);
        french->trigger();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, kTimeoutMs);
        QCOMPARE(translate->label(), u"Translate"_s);
        QCOMPARE(translate->toolTip(), u"Translate: French"_s);
        QVERIFY(w.findChild<QLabel*>(u"outInfo"_s)->text().contains(u"French"_s));
    }

    void writesFromScratch()
    {
        // main-window-compose.html: Write takes a description, a kind and a tone, and keeps
        // its versions like the full reply.
        QVERIFY(m_daemon.start({u"fast-model"_s, u"quality-model"_s}, 32, true));
        MainWindow w;
        w.start();
        auto* write = w.findChild<paraphrase::app::ChipButton*>(u"action-compose"_s);
        QVERIFY(write != nullptr);
        QCOMPARE(write->label(), u"Write"_s);
        write->click(); // nothing described yet: it says so
        w.findChild<QPushButton*>(u"composeMessage"_s)->click();
        QCOMPARE(w.currentSkill(), u"compose.message"_s);
        w.findChild<QPlainTextEdit*>(u"composeText"_s)
            ->setPlainText(u"office closed Monday, standup at 10 am"_s);
        setAnswers({u"Heads up: the office is closed on Monday. Standup at 10 am on video."_s});
        runAndWait(w);
        QVERIFY(w.resultText().contains(u"Monday"_s));
        {
            const QMutexLocker lock(&m_daemon.state->lock);
            // The description and the remembered tone reach the model, whatever the prompt's wording.
            QVERIFY(m_daemon.state->prompts.last().contains(u"office closed Monday"_s));
            QVERIFY(m_daemon.state->prompts.last().contains(u"friendly"_s, Qt::CaseInsensitive));
        }
        const QString info = w.findChild<QLabel*>(u"outInfo"_s)->text();
        QVERIFY2(info.contains(u"kept: "_s) && info.contains(u"Monday"_s), qPrintable(info));
    }

    void optionChangesRunAgain()
    {
        // Owner (2026-10-23): picking Social post after a result did nothing until Ctrl+Enter.
        // Once something ran, a new kind or tone runs again; before that, it waits.
        QVERIFY(m_daemon.start({u"fast-model"_s, u"quality-model"_s}, 32, true));
        MainWindow w;
        w.start();
        w.findChild<paraphrase::app::ChipButton*>(u"action-compose"_s)->click();
        w.findChild<QPushButton*>(u"composeEmail"_s)->click();
        const auto promptCount = [this] {
            const QMutexLocker lock(&m_daemon.state->lock);
            return m_daemon.state->prompts.size();
        };
        auto* text = w.findChild<QPlainTextEdit*>(u"composeText"_s);
        text->setPlainText(u"office closed Monday"_s);
        w.findChild<QPushButton*>(u"composeNote"_s)->click(); // nothing ran yet: no run
        QApplication::processEvents();
        const auto before = promptCount();
        QCOMPARE(w.currentSkill(), u"compose.note"_s);
        setAnswers(
            {u"- Office closed Monday"_s, u"Office closed on Monday! #office"_s, u"Office closed Monday."_s});
        runAndWait(w);
        QCOMPARE(promptCount(), before + 1);

        const QSignalSpy finished(&w, &MainWindow::finished);
        w.findChild<QPushButton*>(u"composePost"_s)->click();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, kTimeoutMs);
        QCOMPARE(w.currentSkill(), u"compose.post"_s);
        QVERIFY(w.resultText().contains(u"#office"_s));

        const auto* tone =
            w.findChild<paraphrase::app::ComposeInput*>()->findChild<paraphrase::app::TonePicker*>();
        QAction* other = nullptr;
        for (QAction* a : tone->menu()->actions()) {
            if (a->objectName() != u"tone-"_s + tone->tone()) {
                other = a;
                break;
            }
        }
        QVERIFY(other != nullptr);
        other->trigger();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 2, kTimeoutMs);
        QCOMPARE(promptCount(), before + 3);

        text->clear(); // cleared: options wait again
        w.findChild<QPushButton*>(u"composeEmail"_s)->click();
        QApplication::processEvents();
        QCOMPARE(promptCount(), before + 3);
    }

    void listsMyActionsAndNewAction()
    {
        // My actions shows before the first action, with New action...; a saved action appears
        // at once (the daemon's SkillsChanged).
        QVERIFY(m_daemon.start({u"fast-model"_s}, 32, true));
        MainWindow w;
        w.start();
        const auto* mine = w.findChild<paraphrase::app::ChipButton*>(u"action-custom"_s);
        QVERIFY(mine != nullptr && mine->menu() != nullptr);
        QCOMPARE(mine->label(), u"My actions"_s);
        QVERIFY(mine->menu()->findChild<QAction*>(u"newAction"_s) != nullptr);
        const QSignalSpy wanted(&w, &MainWindow::newActionRequested);
        mine->menu()->findChild<QAction*>(u"newAction"_s)->trigger();
        QCOMPARE(wanted.size(), 1);

        paraphrase::client::DaemonClient other;
        QVERIFY(other.connectToDaemon().isEmpty());
        (void)other.call(u"SaveAction"_s, {QString(), u"Tweet"_s, u"Turn this into a tweet"_s, u"fast"_s});
        QTRY_VERIFY_WITH_TIMEOUT(w.findChild<QAction*>(u"custom.tweet"_s) != nullptr, kTimeoutMs);
        w.findChild<QPlainTextEdit*>(u"text"_s)->setPlainText(u"We launch on March 12."_s);
        setAnswers({u"Launching March 12!"_s});
        const QSignalSpy finished(&w, &MainWindow::finished);
        w.findChild<QAction*>(u"custom.tweet"_s)->trigger();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, kTimeoutMs);
        QCOMPARE(w.resultText(), u"Launching March 12!"_s);
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
            const QMutexLocker lock(&m_daemon.state->lock);
            m_daemon.state->defaultAnswer = QString(u"word "_s).repeated(200).trimmed();
            m_daemon.state->pieceDelayMs = 20;
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

    void saysSoWhenTheDaemonStopsMidAnswer()
    {
        // A crashed or killed daemon never sends Done: the client ends the request itself.
        startDaemon({u"fast-model"_s});
        {
            const QMutexLocker lock(&m_daemon.state->lock);
            m_daemon.state->defaultAnswer = QString(u"word "_s).repeated(200).trimmed();
            m_daemon.state->pieceDelayMs = 20;
        }
        MainWindow w;
        w.start();
        w.findChild<QPlainTextEdit*>(u"text"_s)->setPlainText(u"Their going home."_s);
        const QSignalSpy finished(&w, &MainWindow::finished);
        w.run();
        QTRY_VERIFY_WITH_TIMEOUT(!outputText(w).isEmpty(), kTimeoutMs);
        m_daemon.state->cancel = true; // lets the fake engine end so the service thread can stop
        m_daemon.stop();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, kTimeoutMs);
        QVERIFY(!w.busy());
        QVERIFY(outputText(w).contains(u"daemon stopped"_s));
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
                    grabNow(w).save(dir + u"/main-window-proofread-%1.png"_s.arg(QLatin1StringView(theme))));
                w.selectSkill(u"reply"_s);
                w.findChild<QPlainTextEdit*>(u"message"_s)
                    ->setPlainText(u"बेटा, रविवार को दादी का जन्मदिन है। सब लोग दोपहर 12 बजे तक घर आ जाना।"_s);
                setAnswers(
                    {uR"({"replies": ["ठीक है, मैं 12 बजे तक आ जाऊँगा।", "क्या मैं कुछ लेकर आऊँ?", "ज़रूर, दादी को मेरी ओर से बधाई!"]})"_s});
                runAndWait(w);
                QVERIFY(grabNow(w).save(dir + u"/main-window-reply-%1.png"_s.arg(QLatin1StringView(theme))));
                if (auto* full = w.findChild<QPushButton*>(u"fullReply"_s);
                    full != nullptr && !full->parentWidget()->isHidden()) {
                    full->click();
                    w.findChild<QPlainTextEdit*>(u"fullMessage"_s)
                        ->setPlainText(u"Hi Priya,\n\nThanks for sending the Q3 vendor report. Could we get "
                                       u"on a call this "
                                       "Thursday at 3 pm to go through the delivery delays? I'd also like "
                                       "the updated cost "
                                       "sheet before then, if possible.\n\nBest,\nDaniel"_s);
                    w.findChild<QPlainTextEdit*>(u"intent"_s)
                        ->setPlainText(
                            u"Thursday doesn't work, offer Friday 11 am. Cost sheet by Wednesday."_s);
                    setAnswers({
                        u"Hi Daniel,\n\nThanks for your email. Unfortunately, Thursday at 3 pm doesn't work "
                        u"for "
                        "me. Would Friday at 11 am suit you instead?\n\nI'll send the updated cost sheet by "
                        "Wednesday, so you'll have it well before we speak.\n\nBest regards,\nPriya"_s,
                    });
                    runAndWait(w);
                    QVERIFY(grabNow(w).save(
                        dir + u"/main-window-full-reply-%1.png"_s.arg(QLatin1StringView(theme))));
                }
                if (auto* write = w.findChild<paraphrase::app::ChipButton*>(u"action-compose"_s)) {
                    write->click();
                    w.findChild<QPlainTextEdit*>(u"composeText"_s)
                        ->setPlainText(
                            u"Tell the team the office is closed on Monday for the plumbing repair. Everyone "
                            "works from home; the standup moves to the video call at 10 am. Thank Ravi for "
                            "sorting it out."_s);
                    setAnswers({
                        u"Subject: Office closed on Monday\n\nHi everyone,\n\nThe office will be closed on "
                        u"Monday "
                        "for a plumbing repair, so we will all work from home that day. The standup moves to "
                        "the "
                        "video call at 10 am.\n\nMany thanks to Ravi for sorting this out.\n\nBest,\n[Your name]"_s,
                    });
                    runAndWait(w);
                    QVERIFY(
                        grabNow(w).save(dir + u"/main-window-write-%1.png"_s.arg(QLatin1StringView(theme))));
                }
            }
            cleanup();
        }
        qunsetenv("PARAPHRASE_THEME");
    }
};

QTEST_MAIN(TestMainWindow)
#include "tst_main_window.moc"
