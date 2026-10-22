// Settings, Models (settings-models.html) against a real daemon service with a fake engine:
// the installed models are listed, Remove asks once in place and removes only after the
// second click, Keep keeps, the speed-up and the tier pickers reach the daemon. With
// PARAPHRASE_SCREENSHOTS=<dir> it saves the page in light and dark.

#include "app_daemon.h"
#include "daemon_client.h"
#include "ui/settings/settings_window.h"

#include <QComboBox>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
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
            DaemonClient client;
            QVERIFY(client.connectToDaemon().isEmpty());
            SettingsWindow w(client);
            w.resize(1180, 1000);
            w.show();
            w.showPage(SettingsWindow::Models);
            QTest::qWait(50);
            QVERIFY(w.grab().save(dir + u"/settings-models-%1.png"_s.arg(QLatin1StringView(theme))));
            m_daemon.stop();
        }
        qunsetenv("PARAPHRASE_THEME");
    }
};

QTEST_MAIN(TestSettingsWindow)
#include "tst_settings_window.moc"
