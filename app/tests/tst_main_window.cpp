// The standalone main window (ADR-013) against a real daemon service with a fake engine, on a
// private socket: the actions come from the skills; proofread shows the word diff and undoes
// one change at a time; reply shows three replies and passes the draft; a tone runs from the
// Tone menu; a missing model and a stopped request are shown in words. With
// PARAPHRASE_SCREENSHOTS=<dir> it also saves the window in light and dark for review.

#include "app_daemon.h"
#include "daemon_client.h"
#include "fake_secrets.h"
#include "ui/common/chip_button.h"
#include "ui/common/grip_splitter.h"
#include "ui/common/menu_icons.h"
#include "ui/main/bars.h"
#include "ui/main/history_view.h"
#include "ui/main/main_window.h"
#include "ui/parts/change_list.h"
#include "ui/parts/input_pages.h"
#include "ui/parts/tone_picker.h"

#include <QAbstractButton>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMutexLocker>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QSplitter>
#include <QSplitterHandle>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBrowser>
#include <QTextEdit>

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

    /// The entries main-window-history.html shows.
    static void seedHistory(paraphrase::app::history::Keeper& keeper)
    {
        const QDate today = QDate::currentDate();
        const QDate yesterday = today.addDays(-1);
        const auto add = [&keeper](const QString& skill, const QString& label, const QString& app,
                                   const QDateTime& when, const QString& text, const QString& result) {
            paraphrase::app::history::Entry e;
            e.skillId = skill;
            e.label = label;
            e.app = app;
            e.when = when;
            e.text = text;
            e.result = result;
            QVERIFY(keeper.record(e) != 0);
        };
        add(u"format.bullets"_s, u"Format · Bullets"_s, {}, QDateTime(yesterday, QTime(9, 5)),
            u"Book the venue, send the invites and order the cake."_s,
            u"- Book the venue\n- Send the invites\n- Order the cake"_s);
        add(u"compose.email"_s, u"Write · Email"_s, {}, QDateTime(yesterday, QTime(18, 30)),
            u"Tell the team the office is closed on Monday."_s,
            u"Subject: Office closed on Monday\n\nHi everyone,\n\nThe office is closed on Monday."_s);
        add(u"reply"_s, u"Reply"_s, u"Telegram"_s, QDateTime(today, QTime(20, 12)), u"Dinner on Friday?"_s,
            u"Friday sounds good!"_s);
        add(u"proofread"_s, u"Proofread · 5 changes"_s, u"KWrite"_s, QDateTime(today, QTime(21, 47)),
            u"Their going to the market tommorow and we needs to buy egg's."_s,
            u"They're going to the market tomorrow and we need to buy eggs."_s);
        add(u"rewrite.elaborate"_s, u"Tone · Elaborate · into Hindi"_s, u"KWrite"_s,
            QDateTime(today, QTime(22, 8)),
            u"Are you free for dinner on Friday? Mia and I were thinking of trying the new Thai place near "
            "the station."_s,
            u"क्या आप शुक्रवार को रात के खाने के लिए उपलब्ध हैं? मैं और Mia स्टेशन के पास उस नए थाई रेस्तरां को "
            "आज़माने के बारे में सोच रहे थे।"_s);
    }

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_settingsDir.isValid());
        // As main.cpp does: menus draw their icons at the size menus.html shows.
        QApplication::setStyle(new paraphrase::app::MenuIconStyle(QApplication::style()->name()));
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_settingsDir.path());
        QStandardPaths::setTestModeEnabled(true); // never the user's history file
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
        w.findChild<QTextEdit*>(u"text"_s)->setPlainText(u"Whisk the egg's with the milk"_s);
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
        // Result: the corrected text alone, the undo kept, no list (approved 2026-10-27).
        w.findChild<QAbstractButton*>(u"showResult"_s)->click();
        QApplication::processEvents();
        QVERIFY(!changes->isVisible());
        QCOMPARE(w.findChild<QTextBrowser*>(u"output"_s)->toPlainText(), u"Whisk the egg's with the milk."_s);
        w.findChild<QAbstractButton*>(u"showChanges"_s)->click();
        QApplication::processEvents();
        QVERIFY(changes->isVisible());
        w.copyResult();
        QCOMPARE(QApplication::clipboard()->text(), u"Whisk the egg's with the milk."_s);
    }

    void showsASpacingChangeAsTheNewSpacing()
    {
        // Owner report: extra blank lines taken out drew as a tall gap of struck-out line
        // breaks in Changes, and the row read "new -> removed, punctuation".
        startDaemon({u"fast-model"_s});
        MainWindow w;
        w.start();
        w.findChild<QTextEdit*>(u"text"_s)->setPlainText(u"See you on Monday.\n\n\n\n\nBring the notes."_s);
        setAnswers({u"See you on Monday.\n\nBring the notes."_s});
        w.selectSkill(u"proofread"_s);
        runAndWait(w);
        const auto* output = w.findChild<QTextBrowser*>(u"output"_s);
        QVERIFY(!output->toPlainText().contains(u"\n\n\n"_s)); // the new spacing, no gap
        const auto reasons = w.findChildren<QLabel*>(QRegularExpression(u"^reason\\d+$"_s));
        QCOMPARE(reasons.size(), 1);
        QCOMPARE(reasons.first()->text(), u"spacing"_s);
        QCOMPARE(paraphrase::app::spacingName(u"\n\n\n\n\n"_s), u"5 line breaks"_s);
        QCOMPARE(paraphrase::app::spacingName(u"\n\n"_s), u"2 line breaks"_s);
        QCOMPARE(paraphrase::app::spacingName(u" "_s), u"a space"_s);
    }

    void namesWhyEachWordChanged()
    {
        // popup-proofread.html, note 3: a short reason per change, from the two words alone.
        using paraphrase::app::changeReason;
        QCOMPARE(changeReason(u"monday"_s, u"Monday"_s), u"capital letter"_s);
        QCOMPARE(changeReason(u"egg's"_s, u"eggs"_s), u"spelling"_s);
        QCOMPARE(changeReason(u"tomorow"_s, u"tomorrow"_s), u"spelling"_s);
        QCOMPARE(changeReason(u"Their"_s, u"They're"_s), u"spelling"_s);
        QCOMPARE(changeReason(QString(), u"."_s), u"full stop"_s);
        QCOMPARE(changeReason(u"fridge"_s, u"fridge?"_s), u"question mark"_s);
        QCOMPARE(changeReason(u"big"_s, u"large"_s), u"wording"_s);
        QCOMPARE(changeReason(QString(), u"the"_s), u"added word"_s);
        QCOMPARE(changeReason(u"very"_s, QString()), u"removed word"_s);
    }

    void replyShowsThreeRepliesAndPassesTheDraft()
    {
        startDaemon({u"fast-model"_s});
        MainWindow w;
        w.start();
        w.selectSkill(u"reply"_s);
        w.findChild<QTextEdit*>(u"message"_s)->setPlainText(u"Are you free for dinner on Friday?"_s);
        w.findChild<QTextEdit*>(u"draft"_s)->setPlainText(u"Yes"_s);
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

    void replyMessageFillsThePaneAndTheSplitResizesIt()
    {
        // main-window-reply.html, note 5: the message takes the pane's height; dragging the
        // split makes the reply so far taller, and the next window keeps it.
        startDaemon({u"fast-model"_s});
        {
            MainWindow w;
            w.resize(1200, 760);
            w.start();
            w.selectSkill(u"reply"_s);
            w.show();
            QVERIFY(QTest::qWaitForWindowExposed(&w));
            const auto* message = w.findChild<QTextEdit*>(u"message"_s);
            const auto* draft = w.findChild<QTextEdit*>(u"draft"_s);
            const auto* split = w.findChild<QSplitter*>(u"replySplit"_s);
            QVERIFY(split != nullptr);
            QCOMPARE(draft->height(), 70); // two lines, the rest is the message's
            QVERIFY(message->height() > 3 * draft->height());

            const auto drag = [split](int dy) {
                QSplitterHandle* handle = split->handle(1);
                const QPoint from = handle->rect().center();
                const QPoint to = from + QPoint(0, dy);
                QTest::mousePress(handle, Qt::LeftButton, {}, from);
                QMouseEvent move(QEvent::MouseMove, QPointF(to), handle->mapToGlobal(QPointF(to)),
                                 Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
                QApplication::sendEvent(handle, &move);
                QTest::mouseRelease(handle, Qt::LeftButton, {}, to);
            };
            drag(-120);
            QTRY_COMPARE(draft->height(), 190);
            // Neither box shrinks below its smallest size: three lines and two lines.
            drag(2000);
            QTRY_COMPARE(draft->height(), 70);
            drag(-2000);
            QTRY_COMPARE(message->height(), 94);
        }
        MainWindow w;
        w.resize(1200, 760);
        w.start();
        w.selectSkill(u"reply"_s);
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        QTRY_VERIFY(w.findChild<QTextEdit*>(u"draft"_s)->height() > 70);
    }

    void bothSplitsShowAGripThatTurnsWithThePanes()
    {
        // main-window.html note 7, main-window-reply.html note 5: grip handles, across when
        // the panes stack in a narrow window.
        using paraphrase::app::GripHandle;
        startDaemon({u"fast-model"_s});
        MainWindow w;
        w.resize(1200, 760);
        w.start();
        w.selectSkill(u"reply"_s);
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        auto* panes = w.findChild<QSplitter*>(u"panes"_s);
        const auto* reply = w.findChild<QSplitter*>(u"replySplit"_s);
        QVERIFY(panes != nullptr && reply != nullptr);
        QVERIFY(qobject_cast<GripHandle*>(reply->handle(1)) != nullptr);
        const auto* grip = qobject_cast<GripHandle*>(panes->handle(1));
        QVERIFY(grip != nullptr);
        QCOMPARE(grip->orientation(), Qt::Horizontal);
        // Under Breeze a proxy takes the pointer over a handle, which then gets only Breeze's
        // HoverEnter and HoverLeave: the grip follows those too.
        QSplitterHandle* handle = reply->handle(1);
        QTRY_VERIFY(!handle->isHidden() && handle->y() > 0);
        const auto gripShown = [handle] {
            const QImage shot = handle->grab().toImage();
            return shot.pixelColor(shot.rect().center()) != shot.pixelColor(0, 0);
        };
        QVERIFY(!gripShown());
        QHoverEvent enter(QEvent::HoverEnter, QPointF(5, 5), QPointF(5, 5), QPointF(-1, -1));
        QApplication::sendEvent(handle, &enter);
        QVERIFY(gripShown());
        QHoverEvent leave(QEvent::HoverLeave, QPointF(-1, -1), QPointF(-1, -1), QPointF(5, 5));
        QApplication::sendEvent(handle, &leave);
        QVERIFY(!gripShown());

        panes->setOrientation(Qt::Vertical);         // what a narrow window does; the offscreen one
        QCOMPARE(grip->orientation(), Qt::Vertical); // cannot get that narrow
    }

    void spacesTheHeaderButtonsEvenly()
    {
        // mock.css .mwhead .btn.flat: 10px padding, 4px between History and Settings.
        startDaemon({u"fast-model"_s});
        MainWindow w;
        w.resize(1200, 760);
        w.start();
        w.findChild<paraphrase::app::HeaderBar*>(u"header"_s)->setHistoryShown(true);
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        const auto* history = w.findChild<QPushButton*>(u"history"_s);
        const auto* settings = w.findChild<QPushButton*>(u"settings"_s);
        QTRY_VERIFY(history->isVisible());
        QCOMPARE(settings->x() - (history->x() + history->width()), 4);
    }

    void splitsShareTheHeightWhereTwoBoxesStack()
    {
        // main-window.html note 8, main-window-history.html, main-window-reply-draft.html
        // note 7: a split wherever two boxes share a column; a drag is kept.
        namespace history = paraphrase::app::history;
        using paraphrase::app::ChangeList;
        startDaemon({u"fast-model"_s, u"quality-model"_s});
        const QTemporaryDir dir;
        auto keeper = fakeHistory(dir.filePath(u"history.sqlite"_s));
        QCOMPARE(keeper->turnOn(), history::Keeper::Problem::None);
        MainWindow w;
        w.setHistory(keeper.get());
        w.resize(1200, 760);
        w.start();
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        const auto drag = [](QSplitter* split, int dy) {
            QSplitterHandle* handle = split->handle(1);
            // A box shown again comes back with its handle at the next layout pass.
            QTRY_VERIFY(!handle->isHidden() && handle->y() > 0);
            const QPoint from = handle->rect().center();
            const QPoint to = from + QPoint(0, dy);
            QTest::mousePress(handle, Qt::LeftButton, {}, from);
            QMouseEvent move(QEvent::MouseMove, QPointF(to), handle->mapToGlobal(QPointF(to)), Qt::NoButton,
                             Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(handle, &move);
            QTest::mouseRelease(handle, Qt::LeftButton, {}, to);
        };

        // Proofread: the list starts as tall as its rows, and a drag makes it taller.
        w.findChild<QTextEdit*>(u"text"_s)->setPlainText(u"Whisk the egg's with the milk"_s);
        setAnswers({u"Whisk the eggs with the milk."_s});
        w.selectSkill(u"proofread"_s);
        runAndWait(w);
        auto* changesSplit = w.findChild<QSplitter*>(u"changesSplit"_s);
        QVERIFY(changesSplit != nullptr);
        const auto* changes = changesSplit->findChild<ChangeList*>(u"changes"_s); // History has one too
        QTRY_COMPARE(changes->height(), changes->fitHeight());
        const int fitted = changes->height();
        drag(changesSplit, -100);
        QTRY_COMPARE(changes->height(), fitted + 100);
        QVERIFY(QSettings().contains(u"proofread/split"_s));

        // History: your text and the result, half each.
        w.showHistory(true);
        auto* historySplit = w.findChild<QSplitter*>(u"historySplit"_s);
        QVERIFY(historySplit != nullptr);
        QTRY_VERIFY(historySplit->isVisible());
        const QList<int> halves = historySplit->sizes();
        QVERIFY(qAbs(halves.at(0) - halves.at(1)) <= 1);
        drag(historySplit, 60);
        QTRY_COMPARE(historySplit->sizes().at(0), halves.at(0) + 60);
        w.showHistory(false);

        // Full reply: the message takes the height, what you want to say starts at two lines.
        w.selectSkill(u"reply"_s);
        auto* full = w.findChild<QPushButton*>(u"fullReply"_s);
        if (full == nullptr || full->parentWidget()->isHidden()) {
            QSKIP("no full reply skill (skills/draft.yaml) in this tree");
        }
        full->click();
        auto* fullSplit = w.findChild<QSplitter*>(u"fullReplySplit"_s);
        const auto* intent = w.findChild<QTextEdit*>(u"intent"_s);
        QTRY_VERIFY(intent->isVisible());
        QCOMPARE(intent->height(), 76);
        QVERIFY(w.findChild<QTextEdit*>(u"fullMessage"_s)->height() > 2 * intent->height());
        drag(fullSplit, -30);
        QTRY_COMPARE(intent->height(), 106);
    }

    void runsAToneFromTheMenu()
    {
        startDaemon({u"fast-model"_s, u"quality-model"_s});
        MainWindow w;
        w.start();
        w.findChild<QTextEdit*>(u"text"_s)->setPlainText(u"send me the file now"_s);
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
        w.findChild<QTextEdit*>(u"text"_s)->setPlainText(u"tell Priya the report is due Friday"_s);
        setAnswers({u"Could you let her know the report is due soon?"_s});
        runAndWait(w);
        QVERIFY(w.findChild<QLabel*>(u"outInfo"_s)->text().contains(u"check: Priya, Friday are missing"_s));
        // The chip keeps its label and size, so the row never re-wraps under the pointer.
        QCOMPARE(tone->label(), u"Tone"_s);
        QCOMPARE(tone->sizeHint().width(), width);
        QCOMPARE(tone->toolTip(), u"Tone: Polite"_s);
        QVERIFY(w.findChild<QLabel*>(u"outInfo"_s)->text().startsWith(u"· polite"_s));
    }

    void takesOverWhatThePopupShowed()
    {
        // popup-open-in-window.html, notes 4 and 5.
        startDaemon({u"fast-model"_s, u"quality-model"_s});
        MainWindow w;
        w.start();
        const QSignalSpy finished(&w, &MainWindow::finished);

        // A reply opens on Full reply with the message, waiting for what to say.
        const QString email = u"Hello everyone,\n\nThe demo is tomorrow at 11.\n\nLuca"_s;
        w.takeOver({.text = email, .skillId = u"reply"_s, .targetLang = {}, .result = {}});
        QCOMPARE(w.currentSkill().section(u'.', 0, 0), u"draft"_s);
        QCOMPARE(w.findChild<QTextEdit*>(u"fullMessage"_s)->toPlainText(), email);
        QVERIFY(w.findChild<QTextEdit*>(u"intent"_s)->toPlainText().isEmpty());
        QVERIFY(!w.findChild<QPushButton*>(u"asChat"_s)->isChecked()); // an email, by its shape

        // Another action opens with the popup's result, without running again.
        w.takeOver({
            .text = u"Their going home."_s,
            .skillId = u"proofread"_s,
            .targetLang = {},
            .result = QJsonObject{{u"output"_s, u"They're going home."_s}},
        });
        QCOMPARE(w.currentSkill(), u"proofread"_s);
        QCOMPARE(w.findChild<QTextEdit*>(u"text"_s)->toPlainText(), u"Their going home."_s);
        QCOMPARE(w.resultText(), u"They're going home."_s);
        QCOMPARE(w.findChild<paraphrase::app::ChipButton*>(u"run"_s)->property("kind").toString(), QString());
        QApplication::processEvents();
        QCOMPARE(finished.size(), 0); // nothing ran
    }

    void runButtonNamesTheActionAndWaitsForText()
    {
        // main-window-run.html: a run button for those who do not use Ctrl+Enter.
        startDaemon({u"fast-model"_s, u"quality-model"_s});
        MainWindow w;
        w.start();
        auto* run = w.findChild<paraphrase::app::ChipButton*>(u"run"_s);
        auto* text = w.findChild<QTextEdit*>(u"text"_s);
        const auto primary = [run] { return run->property("kind").toString() == u"primary"_s; };
        QCOMPARE(run->label(), u"Proofread"_s);
        QVERIFY(!run->isEnabled()); // nothing to run yet
        const int width = run->width();

        // Filled before the first run, plain once its result shows, filled again after an edit.
        text->setPlainText(u"teh cat"_s);
        QVERIFY(run->isEnabled());
        QVERIFY(primary());
        setAnswers({u"The cat."_s});
        const QSignalSpy finished(&w, &MainWindow::finished);
        run->click();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, kTimeoutMs);
        QCOMPARE(w.resultText(), u"The cat."_s);
        QVERIFY(run->isEnabled());
        QVERIFY(!primary());
        text->setPlainText(u"teh dog"_s);
        QVERIFY(primary());

        // Named after the preset; one width whatever the label.
        w.selectSkill(u"rewrite.polite"_s);
        QCOMPARE(run->label(), u"Polite"_s);
        QCOMPARE(run->width(), width);

        // Reply and Write open their form and wait while it is empty.
        text->clear();
        const qsizetype runs = finished.size();
        w.findChild<QPushButton*>(u"action-reply"_s)->click();
        QCOMPARE(w.currentSkill().section(u'.', 0, 0) == u"draft"_s ? u"Write the reply"_s
                                                                    : u"Write 3 replies"_s,
                 run->label());
        QVERIFY(!run->isEnabled());
        w.findChild<QPushButton*>(u"action-compose"_s)->click();
        QCOMPARE(run->label(), u"Write it"_s);
        QVERIFY(!run->isEnabled());
        QApplication::processEvents();
        QCOMPARE(finished.size(), runs); // nothing ran
        QCOMPARE(run->width(), width);
    }

    void switchingReplyKindsMovesNothing()
    {
        // main-window-reply-draft.html: the language and Paste stay in the header for both,
        // and the message starts in the same place, so the switch shifts nothing.
        startDaemon({u"fast-model"_s, u"quality-model"_s});
        MainWindow w;
        w.resize(1200, 760);
        w.start();
        w.selectSkill(u"reply"_s);
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        auto* full = w.findChild<QPushButton*>(u"fullReply"_s);
        if (full == nullptr || full->parentWidget()->isHidden()) {
            QSKIP("no full reply skill (skills/draft.yaml) in this tree");
        }
        w.findChild<QTextEdit*>(u"message"_s)->setPlainText(u"Can we meet on Thursday at 3 pm?"_s);
        const auto* paste = w.findChild<QPushButton*>(u"paste"_s);
        const auto* pill = w.findChild<QLabel*>(u"langPill"_s);
        const auto place = [&w](const QWidget* widget) { return widget->mapTo(&w, QPoint(0, 0)); };
        QTRY_VERIFY(pill->isVisible());
        const QPoint pastePlace = place(paste);
        const QPoint messagePlace = place(w.findChild<QTextEdit*>(u"message"_s));

        full->click();
        const auto* fullMessage = w.findChild<QTextEdit*>(u"fullMessage"_s);
        QCOMPARE(fullMessage->toPlainText(), u"Can we meet on Thursday at 3 pm?"_s); // carried over
        QTRY_VERIFY(fullMessage->isVisible());
        QVERIFY(paste->isVisible());
        QVERIFY(pill->isVisible());
        QCOMPARE(place(paste), pastePlace);
        QCOMPARE(place(fullMessage), messagePlace);
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
        w.findChild<QTextEdit*>(u"fullMessage"_s)->setPlainText(u"Can we meet on Thursday at 3 pm?"_s);
        w.findChild<QTextEdit*>(u"intent"_s)->setPlainText(u"Thursday is bad, offer Friday 11 am"_s);
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
        w.findChild<QTextEdit*>(u"text"_s)->setPlainText(u"The meeting moved to Friday."_s);
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
        w.findChild<QTextEdit*>(u"composeText"_s)->setPlainText(u"office closed Monday, standup at 10 am"_s);
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
        auto* text = w.findChild<QTextEdit*>(u"composeText"_s);
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
        w.findChild<QTextEdit*>(u"text"_s)->setPlainText(u"We launch on March 12."_s);
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
        w.findChild<QTextEdit*>(u"text"_s)->setPlainText(u"teh cat"_s);
        runAndWait(w);
        QVERIFY(outputText(w).contains(u"No model is installed"_s));
    }

    void keepsFinishedRunsInHistory()
    {
        namespace history = paraphrase::app::history;
        startDaemon({u"fast-model"_s});
        const QTemporaryDir dir;
        auto keeper = fakeHistory(dir.filePath(u"history.sqlite"_s));
        MainWindow w;
        w.setHistory(keeper.get());
        w.start();
        auto* text = w.findChild<QTextEdit*>(u"text"_s);
        text->setPlainText(u"Whisk the egg's with the milk"_s);
        w.selectSkill(u"proofread"_s);

        // Off (the default): nothing is kept.
        setAnswers({u"Whisk the eggs with the milk."_s});
        runAndWait(w);
        QVERIFY(keeper->entries().isEmpty());

        QCOMPARE(keeper->turnOn(), history::Keeper::Problem::None);
        setAnswers({u"Whisk the eggs with the milk."_s});
        runAndWait(w);
        QList<history::Entry> kept = keeper->entries();
        QCOMPARE(kept.size(), 1);
        QCOMPARE(kept.at(0).text, u"Whisk the egg's with the milk"_s);
        QCOMPARE(kept.at(0).result, u"Whisk the eggs with the milk."_s);
        QVERIFY(kept.at(0).label.startsWith(u"Proofread"_s));
        QVERIFY(kept.at(0).app.isEmpty()); // the window has no app

        // Retry is another take on the same run: the entry keeps the newest.
        setAnswers({u"Whisk the eggs into the milk."_s});
        const QSignalSpy finished(&w, &MainWindow::finished);
        w.findChild<QPushButton*>(u"retry"_s)->click();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, kTimeoutMs);
        kept = keeper->entries();
        QCOMPARE(kept.size(), 1);
        QCOMPARE(kept.at(0).result, u"Whisk the eggs into the milk."_s);

        // History in place of the panes; search; Use again.
        QVERIFY(!w.historyOpen());
        w.show(); // shortcuts need the active window
        w.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&w));
        QTest::keyClick(&w, Qt::Key_H, Qt::ControlModifier);
        QVERIFY(w.historyOpen());
        const auto* view = w.findChild<paraphrase::app::HistoryView*>();
        QCOMPARE(view->shownCount(), 1);
        QCOMPARE(view->openResult(), u"Whisk the eggs into the milk."_s);
        auto* search = view->findChild<QLineEdit*>(u"historySearch"_s);
        search->setText(u"zebra"_s);
        QCOMPARE(view->shownCount(), 0);
        search->setText(u"MILK"_s);
        QCOMPARE(view->shownCount(), 1);
        view->findChild<QPushButton*>(u"filter-reply"_s)->click();
        QCOMPARE(view->shownCount(), 0);
        view->findChild<QPushButton*>(u"filter-all"_s)->click();
        QCOMPARE(view->shownCount(), 1);
        text->setPlainText(u"something else"_s);
        view->findChild<QPushButton*>(u"useAgain"_s)->click();
        QVERIFY(!w.historyOpen());
        QCOMPARE(text->toPlainText(), u"Whisk the egg's with the milk"_s);
        QCOMPARE(w.currentSkill(), u"proofread"_s);

        // Delete one entry, asked in its header first.
        w.showHistory(true);
        view->findChild<QPushButton*>(u"deleteEntry"_s)->click();
        QCOMPARE(keeper->entries().size(), 1);
        view->findChild<QPushButton*>(u"confirmDeleteEntry"_s)->click();
        QVERIFY(keeper->entries().isEmpty());
        QCOMPARE(view->shownCount(), 0);
    }

    void historyOffSaysSo()
    {
        startDaemon({u"fast-model"_s});
        const QTemporaryDir dir;
        const auto keeper = fakeHistory(dir.filePath(u"history.sqlite"_s));
        MainWindow w;
        w.start();
        QVERIFY(w.findChild<QPushButton*>(u"history"_s)->isHidden()); // no history given: no button
        w.setHistory(keeper.get());
        QVERIFY(!w.findChild<QPushButton*>(u"history"_s)->isHidden());
        w.findChild<QPushButton*>(u"history"_s)->click();
        QVERIFY(w.historyOpen());
        const auto* view = w.findChild<paraphrase::app::HistoryView*>();
        QVERIFY(!view->findChild<QWidget*>(u"historyOff"_s)->isHidden());
        const QSignalSpy privacy(&w, &MainWindow::privacyRequested);
        view->findChild<QPushButton*>(u"turnOnHistory"_s)->click();
        QCOMPARE(privacy.size(), 1);
        view->findChild<QPushButton*>(u"offBack"_s)->click();
        QVERIFY(!w.historyOpen());
    }

    void historyKeepsWindowSize()
    {
        startDaemon({u"fast-model"_s});
        const QTemporaryDir dir;
        const auto keeper = fakeHistory(dir.filePath(u"history.sqlite"_s));
        MainWindow w;
        w.setHistory(keeper.get());
        w.start();
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        for (const bool on : {false, true}) {
            if (on) {
                QCOMPARE(keeper->turnOn(), paraphrase::app::history::Keeper::Problem::None);
                seedHistory(*keeper);
            }
            for (const QSize size : {QSize(1180, 760), w.minimumSize()}) {
                w.resize(size);
                QCoreApplication::processEvents();
                const QSize before = w.size();
                w.showHistory(true);
                QCoreApplication::processEvents();
                QCOMPARE(w.size(), before);
                w.showHistory(false);
                QCoreApplication::processEvents();
                QCOMPARE(w.size(), before);
            }
        }
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
        w.findChild<QTextEdit*>(u"text"_s)->setPlainText(u"Their going home."_s);
        const QSignalSpy finished(&w, &MainWindow::finished);
        w.run();
        QTRY_VERIFY_WITH_TIMEOUT(!outputText(w).isEmpty(), kTimeoutMs); // it streams
        w.cancel();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, kTimeoutMs);
        QVERIFY(!w.busy());
        QVERIFY(w.findChild<QLabel*>(u"outInfo"_s)->text().contains(u"stopped"_s));
    }

    void showsWhatItIsDoingWhileItRuns()
    {
        // main-window-busy.html: the working pill and the shape of the answer until its first
        // word, then the words with the pill saying "Writing"; the summary line comes back after.
        startDaemon({u"fast-model"_s});
        {
            const QMutexLocker lock(&m_daemon.state->lock);
            m_daemon.state->defaultAnswer = QString(u"word "_s).repeated(200).trimmed();
            m_daemon.state->pieceDelayMs = 20;
        }
        MainWindow w;
        w.start();
        w.findChild<QTextEdit*>(u"text"_s)->setPlainText(u"Their going home."_s);
        const auto* pill = w.findChild<QWidget*>(u"busyPill"_s);
        const auto* skeleton = w.findChild<QWidget*>(u"skeleton"_s);
        const auto* info = w.findChild<QLabel*>(u"outInfo"_s);
        QVERIFY(!pill->isVisibleTo(&w));
        const QSignalSpy finished(&w, &MainWindow::finished);
        w.run();
        QVERIFY(pill->isVisibleTo(&w));
        QCOMPARE(pill->accessibleDescription(), u"Reading your text"_s);
        QVERIFY(skeleton->isVisibleTo(&w));
        QVERIFY(!info->isVisibleTo(&w));
        const int width = pill->sizeHint().width();
        // PARAPHRASE_SCREENSHOTS=<dir>: both steps, to compare with the mock.
        const QString dir = qEnvironmentVariable("PARAPHRASE_SCREENSHOTS");
        const QString theme = qEnvironmentVariable("PARAPHRASE_THEME", u"light"_s);
        if (!dir.isEmpty()) {
            w.resize(1180, 720);
            w.show();
            // Laid out, but no socket events yet: the first word must not land before the grab.
            QApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            QVERIFY(w.grab().save(dir + u"/main-window-reading-%1.png"_s.arg(theme)));
        }
        QTRY_VERIFY_WITH_TIMEOUT(outputText(w).size() > 120, kTimeoutMs);
        if (!dir.isEmpty()) {
            QVERIFY(grabNow(w).save(dir + u"/main-window-writing-%1.png"_s.arg(theme)));
        }
        QCOMPARE(pill->accessibleDescription(), u"Writing"_s);
        QCOMPARE(pill->sizeHint().width(), width); // another step, the same width: nothing beside it moves
        QVERIFY(!skeleton->isVisibleTo(&w));
        w.cancel();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, kTimeoutMs);
        QVERIFY(!pill->isVisibleTo(&w));
        QVERIFY(info->isVisibleTo(&w));
    }

    void menusShowAnIconPerItem()
    {
        // menus.html: tones and formats their own icon, languages their script.
        startDaemon({u"fast-model"_s});
        MainWindow w;
        w.start();
        w.findChild<QTextEdit*>(u"text"_s)->setPlainText(u"See you at the station at noon."_s);
        for (const QString& base : {u"rewrite"_s, u"format"_s, u"translate"_s}) {
            QMenu* menu = w.findChild<paraphrase::app::ChipButton*>(u"action-"_s + base)->menu();
            Q_EMIT menu->aboutToShow();
            // Tones and formats are the menu's rows; languages are rows of its list, each
            // with its action ("lang-<code>").
            const auto rows = base == u"translate"_s
                                  ? menu->findChildren<QAction*>(QRegularExpression(u"^lang-"_s))
                                  : menu->actions();
            QVERIFY(!rows.isEmpty());
            for (const QAction* a : rows) {
                QVERIFY2(a->isSeparator() || !a->icon().isNull(), qPrintable(a->objectName()));
            }
        }
        const QMenu* translate = w.findChild<paraphrase::app::ChipButton*>(u"action-translate"_s)->menu();
        QCOMPARE(translate->findChild<QAction*>(u"lang-hi"_s)->property("nativeName").toString(), u"हिन्दी"_s);
        // PARAPHRASE_SCREENSHOTS=<dir>: the menus open, to compare with menus.html.
        if (const QString dir = qEnvironmentVariable("PARAPHRASE_SCREENSHOTS"); !dir.isEmpty()) {
            const QString theme = qEnvironmentVariable("PARAPHRASE_THEME", u"light"_s);
            w.show();
            for (const QString& base : {u"rewrite"_s, u"format"_s, u"translate"_s}) {
                QMenu* menu = w.findChild<paraphrase::app::ChipButton*>(u"action-"_s + base)->menu();
                menu->popup(w.mapToGlobal(QPoint(40, 200)));
                QApplication::processEvents();
                QVERIFY(menu->grab().save(dir + u"/menu-%1-%2.png"_s.arg(base, theme)));
                if (base == u"translate"_s) {
                    QTest::keyClick(menu->findChild<QWidget*>(u"languageList"_s), Qt::Key_End);
                    QApplication::processEvents();
                    QVERIFY(menu->grab().save(dir + u"/menu-translate-end-%1.png"_s.arg(theme)));
                }
                menu->hide();
            }
            w.findChild<paraphrase::app::ChipButton*>(u"action-compose"_s)->click();
            QApplication::processEvents();
            auto* tone =
                w.findChild<paraphrase::app::ComposeInput*>()->findChild<paraphrase::app::TonePicker*>();
            QVERIFY(tone->grab().save(dir + u"/tone-picker-%1.png"_s.arg(theme)));
        }
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
        w.findChild<QTextEdit*>(u"text"_s)->setPlainText(u"Their going home."_s);
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
                w.findChild<QTextEdit*>(u"text"_s)->setPlainText(
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
                w.findChild<QTextEdit*>(u"message"_s)
                    ->setPlainText(u"बेटा, रविवार को दादी का जन्मदिन है। सब लोग दोपहर 12 बजे तक घर आ जाना।"_s);
                setAnswers(
                    {uR"({"replies": ["ठीक है, मैं 12 बजे तक आ जाऊँगा।", "क्या मैं कुछ लेकर आऊँ?", "ज़रूर, दादी को मेरी ओर से बधाई!"]})"_s});
                runAndWait(w);
                QVERIFY(grabNow(w).save(dir + u"/main-window-reply-%1.png"_s.arg(QLatin1StringView(theme))));
                if (auto* full = w.findChild<QPushButton*>(u"fullReply"_s);
                    full != nullptr && !full->parentWidget()->isHidden()) {
                    full->click();
                    w.findChild<QTextEdit*>(u"fullMessage"_s)
                        ->setPlainText(u"Hi Priya,\n\nThanks for sending the Q3 vendor report. Could we get "
                                       u"on a call this "
                                       "Thursday at 3 pm to go through the delivery delays? I'd also like "
                                       "the updated cost "
                                       "sheet before then, if possible.\n\nBest,\nDaniel"_s);
                    w.findChild<QTextEdit*>(u"intent"_s)
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
                    w.findChild<QTextEdit*>(u"composeText"_s)
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
                // main-window-history.html: (b) off, then (a) with one entry open.
                QSettings().remove(u"history"_s); // each theme starts with history off
                const QTemporaryDir historyDir;
                auto keeper = fakeHistory(historyDir.filePath(u"history.sqlite"_s));
                w.setHistory(keeper.get());
                w.showHistory(true);
                QVERIFY(grabNow(w).save(dir +
                                        u"/main-window-history-off-%1.png"_s.arg(QLatin1StringView(theme))));
                w.showHistory(false);
                QCOMPARE(keeper->turnOn(), paraphrase::app::history::Keeper::Problem::None);
                seedHistory(*keeper);
                w.showHistory(true);
                QVERIFY(
                    grabNow(w).save(dir + u"/main-window-history-%1.png"_s.arg(QLatin1StringView(theme))));
                w.showHistory(false);
            }
            cleanup();
        }
        qunsetenv("PARAPHRASE_THEME");
    }
};

QTEST_MAIN(TestMainWindow)
#include "tst_main_window.moc"
