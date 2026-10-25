// The popup flow (PLAN 3.2, popup-*.html) against a real daemon service with a fake engine and
// fake backends: it opens on the captured text, runs an action, and puts the result back only
// when the user clicks Replace; reply shows three replies; Esc closes. With
// PARAPHRASE_SCREENSHOTS=<dir> it also saves its pages in light and dark for review.

#include "app_daemon.h"
#include "fake_secrets.h"
#include "integration/clipboard_backends.h"
#include "integration/integrations.h"
#include "popup/popup_controller.h"
#include "popup/popup_pages.h"
#include "popup/popup_window.h"
#include "ui/common/chip_button.h"

#include <QCoreApplication>
#include <QDir>
#include <QFrame>
#include <QLabel>
#include <QMenu>
#include <QMutexLocker>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;
using namespace paraphrase::app;
using paraphrase::testing::AppDaemon;

namespace {

/// Records what Replace sent instead of typing anywhere.
class RecordingInsert final : public InsertBackend
{
public:
    explicit RecordingInsert(QStringList& sent)
        : m_sent(sent)
    {}
    [[nodiscard]] QString name() const override { return u"recording"_s; }
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] InsertOutcome insert(const QString& text, const CapturedText& /*target*/) override
    {
        m_sent.append(text);
        return InsertOutcome::Typed;
    }

private:
    QStringList& m_sent;
};

CapturedText selected(const QString& text)
{
    return {.text = text, .origin = u"selection"_s, .appName = u"Telegram"_s, .window = u"1"_s};
}

} // namespace

class TestPopup : public QObject
{
    Q_OBJECT

private:
    AppDaemon m_daemon;
    QStringList m_sent;

    [[nodiscard]] std::unique_ptr<Integrations> integrations(QObject* owner)
    {
        std::vector<std::unique_ptr<CaptureBackend>> capture;
        capture.push_back(std::make_unique<ClipboardCapture>());
        std::vector<std::unique_ptr<InsertBackend>> insert;
        insert.push_back(std::make_unique<RecordingInsert>(m_sent));
        return std::make_unique<Integrations>(std::move(capture), std::move(insert),
                                              std::vector<ShortcutBackend*>{new CommandShortcut(owner)});
    }

    static void pick(PopupController& popup, const QString& objectName)
    {
        auto* action = popup.window()->findChild<ChipButton*>(objectName);
        QVERIFY(action != nullptr);
        const QSignalSpy finished(&popup, &PopupController::finished);
        action->click();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, AppDaemon::kTimeoutMs);
    }

    QTemporaryDir m_settingsDir; ///< the tests' own QSettings, never the user's

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_settingsDir.isValid());
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_settingsDir.path());
    }

    void init()
    {
        QSettings().clear(); // the last action of one test never steers the next
        m_sent.clear();
        QVERIFY(m_daemon.start({u"fast-model"_s, u"quality-model"_s}));
    }
    void cleanup() { m_daemon.stop(); }

    void proofreadsAndReplacesOnlyOnClick()
    {
        QObject owner;
        const auto backends = integrations(&owner);
        PopupController popup(*backends);
        popup.showText(selected(u"Mia dont know where the keys are"_s));
        // It reads ahead for the likely action: the working pill says which.
        QVERIFY(popup.window()
                    ->findChild<QWidget*>(u"popupBusy"_s)
                    ->accessibleDescription()
                    .contains(u"Proofread"_s));
        m_daemon.setAnswers({u"Mia doesn't know where the keys are."_s});
        pick(popup, u"action-proofread"_s);
        QVERIFY(m_sent.isEmpty()); // nothing goes back without a click
        auto* insert = popup.window()->findChild<ChipButton*>(u"insert"_s);
        // "Replace in Telegram", or cut with "…" where the fonts run wide (the build
        // container draws boxes): never clipped mid-letter.
        const QString label = insert->label();
        QVERIFY2(label == u"Replace in Telegram"_s || label == u"Replace"_s ||
                     (label.startsWith(u"Replace in Tel"_s) && label.endsWith(u'…')),
                 qPrintable(label));
        insert->click();
        QCOMPARE(m_sent, QStringList{u"Mia doesn't know where the keys are."_s});
    }

    void keepsRunsFromAppsNotLeftOut()
    {
        const QTemporaryDir dir;
        auto keeper = paraphrase::testing::fakeHistory(dir.filePath(u"history.sqlite"_s));
        QCOMPARE(keeper->turnOn(), history::Keeper::Problem::None);
        QObject owner;
        const auto backends = integrations(&owner);
        PopupController popup(*backends);
        popup.setHistory(keeper.get());
        popup.showText(selected(u"Mia dont know where the keys are"_s));
        m_daemon.setAnswers({u"Mia doesn't know where the keys are."_s});
        pick(popup, u"action-proofread"_s);
        const QList<history::Entry> kept = keeper->entries();
        QCOMPARE(kept.size(), 1);
        QCOMPARE(kept.at(0).app, u"Telegram"_s);
        QCOMPARE(kept.at(0).result, u"Mia doesn't know where the keys are."_s);

        // Text from an app the user left out is never kept.
        keeper->setExcludedApps({u"Telegram"_s});
        popup.showText(selected(u"Their going home."_s));
        m_daemon.setAnswers({u"They're going home."_s});
        pick(popup, u"action-proofread"_s);
        QCOMPARE(keeper->entries().size(), 1);
    }

    void saysLooksGoodWhenNothingNeedsFixing()
    {
        QObject owner;
        const auto backends = integrations(&owner);
        PopupController popup(*backends);
        popup.showText(selected(u"Where are the keys?"_s));
        m_daemon.setAnswers({u"Where are the keys?"_s});
        pick(popup, u"action-proofread"_s);
        QCOMPARE(popup.window()->findChild<QLabel*>(u"popupInfo"_s)->text(), u"Looks good, no changes"_s);
        QVERIFY(popup.window()->findChild<ChipButton*>(u"insert"_s)->isHidden());
        QVERIFY(!popup.window()->findChild<QPushButton*>(u"closeResult"_s)->isHidden());
    }

    void offersThreeRepliesAndInsertsTheChosenOne()
    {
        QObject owner;
        const auto backends = integrations(&owner);
        PopupController popup(*backends);
        popup.showText(selected(u"bhaiya parcel aapke ghar ke bahar rakh du?"_s));
        m_daemon.setAnswers(
            {uR"({"replies": ["Haan, rakh do.", "Thoda ruko.", "Nahi, main aa raha hoon."]})"_s});
        pick(popup, u"action-reply"_s);
        popup.window()->findChild<QPushButton*>(u"reply2"_s)->click();
        popup.window()->findChild<ChipButton*>(u"insert"_s)->click();
        QCOMPARE(m_sent, QStringList{u"Thoda ruko."_s});
    }

    void rewritesInAnotherLanguage()
    {
        QObject owner;
        const auto backends = integrations(&owner);
        PopupController popup(*backends);
        popup.showText(selected(u"send me the report by friday"_s));
        m_daemon.setAnswers({u"Could you send me the report by Friday?"_s});
        const auto* tone = popup.window()->findChild<ChipButton*>(u"action-rewrite"_s);
        QVERIFY(tone != nullptr);
        const QSignalSpy finished(&popup, &PopupController::finished);
        tone->menu()->findChild<QAction*>(u"rewrite.polite"_s)->trigger();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, AppDaemon::kTimeoutMs);
        const auto* other = popup.window()->findChild<QPushButton*>(u"otherLanguage"_s);
        QVERIFY(other != nullptr && !other->isHidden());
        Q_EMIT other->menu()->aboutToShow();
        QVERIFY(other->menu()->findChild<QAction*>(u"lang-en"_s) == nullptr); // its own language
        m_daemon.setAnswers({u"क्या आप शुक्रवार तक रिपोर्ट भेज सकते हैं?"_s});
        other->menu()->findChild<QAction*>(u"lang-hi"_s)->trigger();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 2, AppDaemon::kTimeoutMs);
        {
            const QMutexLocker lock(&m_daemon.state->lock);
            QVERIFY(m_daemon.state->prompts.last().contains(u"Hindi"_s));
        }
        // popup-working.html, 3 and 4: the chosen language as a pill, no false "Friday is
        // missing" across languages, and × goes back to the text's own language.
        const auto* chosen = popup.window()->findChild<QFrame*>(u"chosenLanguage"_s);
        QVERIFY(chosen != nullptr && !chosen->isHidden());
        QVERIFY(popup.window()->findChild<QLabel*>(u"chosenLanguageName"_s)->text().contains(u"Hindi"_s));
        QVERIFY(!popup.window()->findChild<QLabel*>(u"popupInfo"_s)->text().contains(u"missing"_s));
        m_daemon.setAnswers({u"Could you send me the report by Friday?"_s});
        popup.window()->findChild<QPushButton*>(u"clearLanguage"_s)->click();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 3, AppDaemon::kTimeoutMs);
        QVERIFY(chosen->isHidden());
        const QMutexLocker lock(&m_daemon.state->lock);
        QVERIFY(!m_daemon.state->prompts.last().contains(u"chose Hindi"_s));
    }

    void saysWhatATonedChanged()
    {
        // popup-working.html, 4: the tone, and how the length moved for Elaborate.
        QObject owner;
        const auto backends = integrations(&owner);
        PopupController popup(*backends);
        popup.showText(selected(u"Meet on Friday at 3 pm."_s));
        m_daemon.setAnswers(
            {u"Could we meet on Friday at 3 pm, if that suits you and the rest of the team?"_s});
        const auto* tone = popup.window()->findChild<ChipButton*>(u"action-rewrite"_s);
        const QSignalSpy finished(&popup, &PopupController::finished);
        tone->menu()->findChild<QAction*>(u"rewrite.elaborate"_s)->trigger();
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, AppDaemon::kTimeoutMs);
        const QString line = popup.window()->findChild<QLabel*>(u"popupInfo"_s)->text();
        QVERIFY2(line.startsWith(u"Elaborate"_s) && line.contains(u"6 to 18 words"_s), qPrintable(line));
    }

    void showsItIsWorkingAhead()
    {
        // popup-working.html, 2: the footer's working pill while it reads ahead.
        QObject owner;
        const auto backends = integrations(&owner);
        PopupController popup(*backends);
        popup.showText(selected(u"Their going home."_s));
        const auto* busy = popup.window()->findChild<QWidget*>(u"popupBusy"_s);
        QVERIFY(busy != nullptr && !busy->isHidden());
        QTRY_VERIFY_WITH_TIMEOUT(busy->isHidden(), 5000); // the fake daemon is quick: then "Ready for"
        QVERIFY(popup.window()->findChild<QLabel*>(u"popupStatus"_s)->text().startsWith(u"Ready for"_s));
    }

    void escCloses()
    {
        QObject owner;
        const auto backends = integrations(&owner);
        PopupController popup(*backends);
        popup.showText(selected(u"some text"_s));
        popup.window()->show();
        QTest::keyClick(popup.window(), Qt::Key_Escape);
        QVERIFY(!popup.window()->isVisible());
    }

    void savesScreenshots()
    {
        const QString dir = qEnvironmentVariable("PARAPHRASE_SCREENSHOTS");
        if (dir.isEmpty()) {
            QSKIP("set PARAPHRASE_SCREENSHOTS=<dir> to save the popup in light and dark");
        }
        QDir().mkpath(dir);
        for (const char* theme : {"light", "dark"}) {
            qputenv("PARAPHRASE_THEME", theme);
            QObject owner;
            const auto backends = integrations(&owner);
            PopupController popup(*backends);
            const QString name = QLatin1StringView(theme);
            popup.showText(
                selected(u"Mia dont know where the keys are, can you check the drawer next to the fridge"_s));
            popup.window()->show();
            QCoreApplication::processEvents(); // queued shows land first
            QVERIFY(popup.window()->grab().save(dir + u"/popup-actions-%1.png"_s.arg(name)));
            m_daemon.setAnswers(
                {u"Mia doesn't know where the keys are, can you check the drawer next to the fridge?"_s});
            pick(popup, u"action-proofread"_s);
            QTest::qWait(100); // the result box and its rows settle, and the popup fits them
            QVERIFY(popup.window()->grab().save(dir + u"/popup-proofread-%1.png"_s.arg(name)));
            popup.showText(
                selected(u"bhaiya parcel aapke ghar ke bahar rakh du? koi phone nahi utha raha"_s));
            m_daemon.setAnswers({
                uR"({"replies": ["Haan, rakh do main check karta hoon.", "Thoda time do, main call karta hoon.", "Abhi nahi, main khud utha lunga."]})"_s,
            });
            pick(popup, u"action-reply"_s);
            QCoreApplication::processEvents(); // queued shows land first
            QVERIFY(popup.window()->grab().save(dir + u"/popup-reply-%1.png"_s.arg(name)));
        }
        qunsetenv("PARAPHRASE_THEME");
    }
};

QTEST_MAIN(TestPopup)
#include "tst_popup.moc"
