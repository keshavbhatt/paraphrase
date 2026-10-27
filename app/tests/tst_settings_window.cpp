// Settings, Models (settings-models.html) against a real daemon service with a fake engine:
// the installed models are listed, Remove asks once in place and removes only after the
// second click, Keep keeps, the speed-up and the tier pickers reach the daemon. With
// PARAPHRASE_SCREENSHOTS=<dir> it saves the page in light and dark.

#include "app_daemon.h"
#include "daemon_client.h"
#include "fake_secrets.h"
#include "integration/launch_command.h"
#include "ui/common/elided_label.h"
#include "ui/common/menu_icons.h"
#include "ui/common/toggle_switch.h"
#include "ui/core/preferences.h"
#include "ui/settings/settings_window.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMutexLocker>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;
using paraphrase::app::SettingsWindow;
using paraphrase::client::DaemonClient;
using paraphrase::testing::AppDaemon;

namespace {

QJsonObject status(DaemonClient& client)
{
    return QJsonDocument::fromJson(client.call(u"Status"_s, {}).arguments().value(0).toString().toUtf8())
        .object();
}

bool installed(DaemonClient& client, const QString& id)
{
    return status(client).value(u"installed"_s).toArray().contains(id);
}

} // namespace

class TestSettingsWindow : public QObject
{
    Q_OBJECT

private:
    AppDaemon m_daemon;
    QTemporaryDir m_settingsDir; ///< the tests' own QSettings, never the user's

    /// settings-privacy.html: (a) off, (b) on with entries, (c) Delete all asked on the page.
    static void savePrivacy(DaemonClient& client, const QString& dir, QLatin1StringView theme)
    {
        namespace history = paraphrase::app::history;
        QSettings().remove(u"history"_s);
        const QTemporaryDir historyDir;
        auto keeper = paraphrase::testing::fakeHistory(historyDir.filePath(u"history.sqlite"_s));
        SettingsWindow w(client, nullptr, false, keeper.get());
        w.resize(1180, 820);
        w.show();
        w.showPage(SettingsWindow::Privacy);
        const auto grab = [&w, &dir, theme](const QString& name) {
            QTest::qWait(50);
            QCoreApplication::processEvents();
            QVERIFY(w.grab().save(dir + u"/settings-privacy-%1-%2.png"_s.arg(name, theme)));
        };
        grab(u"off"_s);
        w.findChild<paraphrase::app::ToggleSwitch*>(u"keepHistory"_s)->click();
        for (int i = 0; i < 3; ++i) {
            history::Entry e;
            e.skillId = u"proofread"_s;
            e.label = u"Proofread"_s;
            e.when = QDateTime::currentDateTime().addDays(-2LL * i);
            e.text = u"Their going home."_s;
            e.result = u"They're going home."_s;
            keeper->record(e);
        }
        grab(u"on"_s);
        w.findChild<QPushButton*>(u"deleteAllHistory"_s)->click();
        grab(u"confirm"_s);
        QSettings().remove(u"history"_s);
    }

private Q_SLOTS:
    void initTestCase()
    {
        // Never the user's settings or history file.
        QStandardPaths::setTestModeEnabled(true);
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_settingsDir.path());
        // As main.cpp does: menus and drop-down lists with round corners.
        QApplication::setStyle(new paraphrase::app::MenuIconStyle(QApplication::style()->name()));
    }

    void cleanup() { m_daemon.stop(); }
    static void init() { QSettings().clear(); }

    void privacyTurnsHistoryOnAndAsksBeforeDeleting()
    {
        using paraphrase::app::ToggleSwitch;
        namespace history = paraphrase::app::history;
        QVERIFY(m_daemon.start({u"fast-model"_s}));
        DaemonClient client;
        QVERIFY(client.connectToDaemon().isEmpty());
        const QTemporaryDir dir;
        const QString path = dir.filePath(u"history.sqlite"_s);
        auto keeper = paraphrase::testing::fakeHistory(path);
        SettingsWindow w(client, nullptr, false, keeper.get());
        w.showPage(SettingsWindow::Privacy);
        auto* on = w.findChild<ToggleSwitch*>(u"keepHistory"_s);
        QVERIFY(on != nullptr && !on->isChecked());                       // off by default (PLAN D13)
        QVERIFY(!w.findChild<QComboBox*>(u"keepFor"_s)->isVisibleTo(&w)); // the options show once on
        on->click();
        QVERIFY(history::Keeper::isOn());
        QCOMPARE(w.findChild<QComboBox*>(u"keepFor"_s)->currentText(), u"30 days"_s);
        QVERIFY(w.findChild<QPushButton*>(u"excluded-KeePassXC"_s) != nullptr);

        history::Entry e;
        e.skillId = u"proofread"_s;
        e.label = u"Proofread"_s;
        e.text = u"teh text"_s;
        e.result = u"the text"_s;
        QVERIFY(keeper->record(e) != 0);
        QVERIFY(w.findChild<QLabel*>(u"historyKept"_s)->text().startsWith(u"1 entry, from"_s));

        // Delete all asks on the page first; Keep them keeps them.
        const auto* confirm = w.findChild<QFrame*>(u"confirm"_s);
        QVERIFY(confirm->isHidden());
        w.findChild<QPushButton*>(u"deleteAllHistory"_s)->click();
        QVERIFY(!confirm->isHidden());
        QCOMPARE(keeper->stats().count, 1);
        w.findChild<QPushButton*>(u"keepThem"_s)->click();
        QVERIFY(confirm->isHidden());
        QCOMPARE(keeper->stats().count, 1);
        w.findChild<QPushButton*>(u"deleteAllHistory"_s)->click();
        w.findChild<QPushButton*>(u"confirmDeleteAll"_s)->click();
        QCOMPARE(keeper->stats().count, 0);
        QVERIFY(history::Keeper::isOn()); // stays on for what comes next

        // Turning off with entries kept asks whether to delete them, keeping them by default.
        keeper->record(e);
        on->click();
        QVERIFY(!history::Keeper::isOn());
        QVERIFY(!confirm->isHidden());
        w.findChild<QPushButton*>(u"keepThem"_s)->click();
        QVERIFY(QFile::exists(path));

        // An app added by hand is left out.
        on->click();
        w.findChild<QPushButton*>(u"addApp"_s)->click();
        auto* name = w.findChild<QLineEdit*>(u"appName"_s);
        name->setText(u"Signal"_s);
        QTest::keyClick(name, Qt::Key_Return);
        QVERIFY(history::Keeper::isExcluded(u"signal"_s));
    }

    void privacyWithoutAKeyringSaysWhy()
    {
        using paraphrase::app::ToggleSwitch;
        namespace history = paraphrase::app::history;
        QVERIFY(m_daemon.start({u"fast-model"_s}));
        DaemonClient client;
        QVERIFY(client.connectToDaemon().isEmpty());
        const QTemporaryDir dir;
        history::Keeper keeper(dir.filePath(u"history.sqlite"_s),
                               [](const QString& /*kind*/) { return nullptr; });
        SettingsWindow w(client, nullptr, false, &keeper);
        w.showPage(SettingsWindow::Privacy);
        auto* on = w.findChild<ToggleSwitch*>(u"keepHistory"_s);
        on->click();
        QVERIFY(!on->isChecked());
        QVERIFY(!history::Keeper::isOn());
        const auto* problem = w.findChild<QLabel*>(u"historyProblem"_s);
        QVERIFY(!problem->isHidden());
        QVERIFY(problem->text().contains(u"keyring"_s));
    }

    void listsTheInstalledModels()
    {
        QVERIFY(m_daemon.start({u"fast-model"_s, u"quality-model"_s, u"fast-draft"_s}));
        DaemonClient client;
        QVERIFY(client.connectToDaemon().isEmpty());
        SettingsWindow w(client);
        w.showPage(SettingsWindow::Models);
        QVERIFY(w.findChild<QPushButton*>(u"remove-fast-model"_s) != nullptr);
        QVERIFY(w.findChild<QPushButton*>(u"remove-quality-model"_s) != nullptr);
        QCOMPARE(w.findChild<QLabel*>(u"state-fast-model"_s)->text(), u"Ready, verified"_s);
        QCOMPARE(w.findChild<QLabel*>(u"speedUpState"_s)->text(), u"On"_s);
        QVERIFY(w.findChild<QLabel*>(u"modelsFolder"_s)->text().contains(u"/models"_s));
    }

    void removesOnlyAfterConfirmation()
    {
        QVERIFY(m_daemon.start({u"fast-model"_s, u"quality-model"_s}));
        DaemonClient client;
        QVERIFY(client.connectToDaemon().isEmpty());
        SettingsWindow w(client);
        w.showPage(SettingsWindow::Models);
        // The first click only asks.
        w.findChild<QPushButton*>(u"remove-quality-model"_s)->click();
        QVERIFY(installed(client, u"quality-model"_s));
        auto* keep = w.findChild<QPushButton*>(u"keep-quality-model"_s);
        QVERIFY(keep != nullptr);
        keep->click();
        QVERIFY(installed(client, u"quality-model"_s));
        QTRY_VERIFY(w.findChild<QPushButton*>(u"keep-quality-model"_s) == nullptr);
        // Asked again and confirmed: gone.
        w.findChild<QPushButton*>(u"remove-quality-model"_s)->click();
        w.findChild<QPushButton*>(u"confirmRemove-quality-model"_s)->click();
        QVERIFY(!installed(client, u"quality-model"_s));
        QVERIFY(installed(client, u"fast-model"_s));
    }

    void choicesReachTheDaemon()
    {
        QVERIFY(m_daemon.start({u"fast-model"_s, u"quality-model"_s, u"fast-draft"_s}));
        DaemonClient client;
        QVERIFY(client.connectToDaemon().isEmpty());
        SettingsWindow w(client);
        w.showPage(SettingsWindow::Models);
        w.findChild<QPushButton*>(u"speedUp"_s)->click();
        QVERIFY(!status(client).value(u"speed_up"_s).toBool());
        QTRY_COMPARE(w.findChild<QLabel*>(u"speedUpState"_s)->text(), u"Off"_s);
        auto* quality = w.findChild<QComboBox*>(u"tierQuality"_s);
        const int fast = quality->findData(u"fast-model"_s);
        QVERIFY(fast >= 0);
        quality->setCurrentIndex(fast);
        Q_EMIT quality->activated(fast);
        QCOMPARE(status(client).value(u"tier_models"_s).toObject().value(u"quality"_s).toString(),
                 u"fast-model"_s);
    }

    void makesTriesSavesAndDeletesAnAction()
    {
        // settings-actions.html: a new action is tried before saving, then listed; Delete asks
        // once in place.
        QVERIFY(m_daemon.start({u"fast-model"_s, u"quality-model"_s}, 32, true));
        DaemonClient client;
        QVERIFY(client.connectToDaemon().isEmpty());
        SettingsWindow w(client);
        w.show();
        w.newAction();
        w.findChild<QLineEdit*>(u"actionName"_s)->setText(u"Tweet"_s);
        w.findChild<QPlainTextEdit*>(u"actionInstruction"_s)->setPlainText(u"Turn this into a tweet"_s);
        m_daemon.setAnswers({u"Launch moved to March 12! #launch"_s});
        w.findChild<QPushButton*>(u"tryAction"_s)->click();
        QTRY_COMPARE_WITH_TIMEOUT(w.findChild<QLabel*>(u"tryResult"_s)->text(),
                                  u"Launch moved to March 12! #launch"_s, AppDaemon::kTimeoutMs);
        {
            const QMutexLocker lock(&m_daemon.state->lock);
            QVERIFY(m_daemon.state->prompts.last().contains(u"Turn this into a tweet"_s));
        }
        // A long result scrolls in a capped area instead of stretching the editor.
        const auto* area = w.findChild<QScrollArea*>(u"tryResultArea"_s);
        const int shortHeight = area->height();
        m_daemon.setAnswers({QString(u"A much longer answer that goes on. "_s).repeated(80)});
        w.findChild<QPushButton*>(u"tryAction"_s)->click();
        QTRY_VERIFY_WITH_TIMEOUT(w.findChild<QLabel*>(u"tryResult"_s)->text().startsWith(u"A much"_s),
                                 AppDaemon::kTimeoutMs);
        QVERIFY(area->height() > shortHeight);
        QVERIFY(area->height() <= 160);
        QVERIFY(area->verticalScrollBar()->maximum() > 0);
        w.findChild<QPushButton*>(u"saveAction"_s)->click();
        QVERIFY(client.call(u"ListSkills"_s, {}).arguments().value(0).toString().contains(u"custom.tweet"_s));
        auto* remove = w.findChild<QPushButton*>(u"delete-tweet"_s);
        QVERIFY(remove != nullptr);
        remove->click(); // asks
        QVERIFY(client.call(u"ListSkills"_s, {}).arguments().value(0).toString().contains(u"custom.tweet"_s));
        remove->click(); // deletes
        QTRY_VERIFY(
            !client.call(u"ListSkills"_s, {}).arguments().value(0).toString().contains(u"custom.tweet"_s));
    }

    void showsGeneralAndShortcut()
    {
        // settings-general-shortcut.html: the switches keep their state, and the Shortcut page
        // shows the command to bind by hand when the desktop has no global shortcut.
        QVERIFY(m_daemon.start({u"fast-model"_s}));
        DaemonClient client;
        QVERIFY(client.connectToDaemon().isEmpty());
        paraphrase::app::Preferences preferences;
        const QSignalSpy keep(&preferences, &paraphrase::app::Preferences::keepInTrayChanged);
        SettingsWindow w(client, &preferences, true);
        w.show();
        w.showPage(SettingsWindow::General);
        auto* tray = w.findChild<QAbstractButton*>(u"keepInTray"_s);
        QVERIFY(tray != nullptr);
        tray->click();
        QCOMPARE(keep.size(), 1);
        paraphrase::app::DesktopSupport support;
        support.steps.append(
            {.step = u"Open over any app"_s, .how = u"by hand"_s, .status = u"By hand"_s, .works = false});
        w.setDesktopSupport(support);
        w.showPage(SettingsWindow::Shortcut);
        const auto* command = w.findChild<paraphrase::app::ElidedLabel*>(u"popupCommand"_s);
        QVERIFY(command != nullptr);
        QCOMPARE(command->fullText(), paraphrase::app::launchLine({u"--popup"_s}));
        auto* copy = w.findChild<QPushButton*>(u"copyCommand"_s);
        QVERIFY(copy != nullptr);
        copy->click();
        QCOMPARE(QGuiApplication::clipboard()->text(), paraphrase::app::launchLine({u"--popup"_s}));
        tray->click(); // leave the preference as it was
    }

    void letsTheUserChooseTheKeys()
    {
        // settings-general-shortcut.html: Change keys next to the keys, Choose keys when the
        // desktop gave the shortcut none; both open the desktop's window.
        QVERIFY(m_daemon.start({}));
        DaemonClient client;
        QVERIFY(client.connectToDaemon().isEmpty());
        SettingsWindow w(client);
        int chosen = 0;
        paraphrase::app::DesktopSupport support;
        support.globalShortcut = true;
        support.canChangeKeys = true;
        support.chooseKeys = [&chosen] { ++chosen; };
        // The page's cards only: "Put the result back" there keeps a hidden copy of setup's row.
        const auto shown = [&w](const QString& name) -> QPushButton* {
            QCoreApplication::processEvents(); // new cards show, old ones go, on the next pass
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            for (QPushButton* b : w.findChildren<QPushButton*>(name)) {
                if (b->isVisible()) {
                    return b;
                }
            }
            return nullptr;
        };
        w.setDesktopSupport(support);
        w.show();
        w.showPage(SettingsWindow::Shortcut);
        auto* choose = shown(u"chooseKeys"_s);
        QVERIFY(choose != nullptr);
        choose->click();
        QCOMPARE(chosen, 1);
        support.shortcut = u"Ctrl+Alt+Space"_s;
        w.setDesktopSupport(support);
        QVERIFY(shown(u"chooseKeys"_s) == nullptr);
        auto* change = shown(u"changeKeys"_s);
        QVERIFY(change != nullptr);
        change->click();
        QCOMPARE(chosen, 2);
        support.canChangeKeys = false; // no window of its own: the text says where instead
        w.setDesktopSupport(support);
        QVERIFY(shown(u"changeKeys"_s) == nullptr);
    }

    void pagesFitTheWindowAtItsOpeningSize()
    {
        // Wider than its column, a page was cut off at the right under the scroll bar.
        QVERIFY(m_daemon.start({u"fast-model"_s, u"quality-model"_s, u"fast-draft"_s}));
        DaemonClient client;
        QVERIFY(client.connectToDaemon().isEmpty());
        SettingsWindow w(client);
        if (!QFontMetrics(w.font()).inFont(u'a')) {
            QSKIP("no fonts here: every letter is a wide box, so widths mean nothing");
        }
        w.show();
        // Each page as it is shown: a hidden page's scroll area is not laid out.
        for (const auto page : {
                 SettingsWindow::General,
                 SettingsWindow::Shortcut,
                 SettingsWindow::MyActions,
                 SettingsWindow::Models,
                 SettingsWindow::Privacy,
                 SettingsWindow::About,
             }) {
            w.showPage(page);
            QTest::qWait(50);
            const auto* area =
                qobject_cast<const QScrollArea*>(w.findChild<QStackedWidget*>()->currentWidget());
            QVERIFY(area != nullptr);
            const int needs = area->widget()->minimumSizeHint().width();
            QVERIFY2(needs <= area->viewport()->width(),
                     qPrintable(u"%1 needs %2 px, has %3"_s
                                    .arg(QLatin1StringView(area->widget()->metaObject()->className()))
                                    .arg(needs)
                                    .arg(area->viewport()->width())));
        }
        m_daemon.stop();
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
            QVERIFY(m_daemon.start({u"fast-model"_s, u"quality-model"_s, u"fast-draft"_s}));
            {
                DaemonClient client;
                QVERIFY(client.connectToDaemon().isEmpty());
                SettingsWindow w(client);
                w.resize(1180, 1000);
                w.show();
                w.showPage(SettingsWindow::Models);
                QTest::qWait(50);
                QCoreApplication::processEvents(); // queued shows land first
                QVERIFY(w.grab().save(dir + u"/settings-models-%1.png"_s.arg(QLatin1StringView(theme))));
                // At the size it opens at, where the page scrolls: the scroll bar and its gaps.
                w.resize(1080, 780);
                QTest::qWait(50);
                QVERIFY(
                    w.grab().save(dir + u"/settings-models-scroll-%1.png"_s.arg(QLatin1StringView(theme))));
                w.resize(1180, 1000);
                // A drop-down's list has the round corners of the menus (menus.html).
                auto* quality = w.findChild<QComboBox*>(u"tierQuality"_s);
                quality->showPopup();
                QTest::qWait(100);
                QVERIFY(quality->view()->window()->grab().save(
                    dir + u"/settings-combo-%1.png"_s.arg(QLatin1StringView(theme))));
                quality->hidePopup();
            }
            m_daemon.stop();
            QVERIFY(m_daemon.start({u"fast-model"_s}, 32, true));
            DaemonClient actionsClient;
            QVERIFY(actionsClient.connectToDaemon().isEmpty());
            (void)actionsClient.call(u"SaveAction"_s,
                                     {
                                         QString(),
                                         u"Tweet"_s,
                                         u"Turn this into a tweet under 280 characters, keep the facts"_s,
                                         u"fast"_s,
                                     });
            (void)actionsClient.call(u"SaveAction"_s,
                                     {
                                         QString(),
                                         u"Meeting notes"_s,
                                         u"Turn these notes into decisions and action items with owners"_s,
                                         u"quality"_s,
                                     });
            SettingsWindow actions(actionsClient);
            actions.resize(1180, 1000);
            actions.show();
            actions.newAction();
            QTest::qWait(50);
            QCoreApplication::processEvents(); // queued shows land first
            QVERIFY(actions.grab().save(dir + u"/settings-actions-%1.png"_s.arg(QLatin1StringView(theme))));
            paraphrase::app::DesktopSupport support;
            support.globalShortcut = true;
            support.shortcut = u"Ctrl+Alt+Space"_s;
            support.changeIt = u"System Settings, Shortcuts"_s;
            support.steps = {
                {
                    .step = u"Open over any app"_s,
                    .how = u"Shortcut from the desktop"_s,
                    .status = u"Works"_s,
                    .works = true,
                },
                {
                    .step = u"Read the selected text"_s,
                    .how = u"The selection, without copying"_s,
                    .status = u"Works"_s,
                    .works = true,
                },
                {
                    .step = u"Put the result back"_s,
                    .how = u"Pasted into the app, after you allow it once"_s,
                    .status = u"Asks once"_s,
                    .works = true,
                },
            };
            actions.setDesktopSupport(support);
            actions.showPage(SettingsWindow::Shortcut);
            QTest::qWait(50);
            QCoreApplication::processEvents(); // queued shows land first
            QVERIFY(actions.grab().save(dir + u"/settings-shortcut-%1.png"_s.arg(QLatin1StringView(theme))));
            support.canChangeKeys = true;
            support.chooseKeys = [] {};
            actions.setDesktopSupport(support);
            QTest::qWait(50);
            QCoreApplication::processEvents();
            QVERIFY(actions.grab().save(dir +
                                        u"/settings-shortcut-change-%1.png"_s.arg(QLatin1StringView(theme))));
            support.shortcut.clear();
            support.steps.first().status = u"No keys"_s;
            support.steps.first().works = false;
            actions.setDesktopSupport(support);
            QTest::qWait(50);
            QCoreApplication::processEvents();
            QVERIFY(actions.grab().save(dir +
                                        u"/settings-shortcut-nokeys-%1.png"_s.arg(QLatin1StringView(theme))));
            actions.showPage(SettingsWindow::General);
            QTest::qWait(50);
            QCoreApplication::processEvents(); // queued shows land first
            QVERIFY(actions.grab().save(dir + u"/settings-general-%1.png"_s.arg(QLatin1StringView(theme))));
            savePrivacy(actionsClient, dir, QLatin1StringView(theme));
            m_daemon.stop();
        }
        qunsetenv("PARAPHRASE_THEME");
    }
};

QTEST_MAIN(TestSettingsWindow)
#include "tst_settings_window.moc"
