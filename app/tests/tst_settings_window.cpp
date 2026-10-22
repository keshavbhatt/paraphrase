// Settings, Models (settings-models.html) against a real daemon service with a fake engine:
// the installed models are listed, Remove asks once in place and removes only after the
// second click, Keep keeps, the speed-up and the tier pickers reach the daemon. With
// PARAPHRASE_SCREENSHOTS=<dir> it saves the page in light and dark.

#include "app_daemon.h"
#include "daemon_client.h"
#include "ui/core/preferences.h"
#include "ui/settings/settings_window.h"

#include <QAbstractButton>
#include <QClipboard>
#include <QComboBox>
#include <QDir>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMutexLocker>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalSpy>
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

private Q_SLOTS:
    void cleanup() { m_daemon.stop(); }

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
        const auto* command = w.findChild<QLabel*>(u"popupCommand"_s);
        QVERIFY(command != nullptr);
        QCOMPARE(command->text(), u"paraphrase-app --popup"_s);
        auto* copy = w.findChild<QPushButton*>(u"copyCommand"_s);
        QVERIFY(copy != nullptr);
        copy->click();
        QCOMPARE(QGuiApplication::clipboard()->text(), u"paraphrase-app --popup"_s);
        tray->click(); // leave the preference as it was
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
                QVERIFY(w.grab().save(dir + u"/settings-models-%1.png"_s.arg(QLatin1StringView(theme))));
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
            QVERIFY(actions.grab().save(dir + u"/settings-shortcut-%1.png"_s.arg(QLatin1StringView(theme))));
            actions.showPage(SettingsWindow::General);
            QTest::qWait(50);
            QVERIFY(actions.grab().save(dir + u"/settings-general-%1.png"_s.arg(QLatin1StringView(theme))));
            m_daemon.stop();
        }
        qunsetenv("PARAPHRASE_THEME");
    }
};

QTEST_MAIN(TestSettingsWindow)
#include "tst_settings_window.moc"
