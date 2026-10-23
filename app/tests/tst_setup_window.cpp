// The first-run window (setup-model.html) against a real daemon service with a fake engine:
// Fast is preselected and Quality optional, nothing downloads before a click, the download
// panel follows the daemon's progress, Continue waits for an installed model, and "Try it"
// proofreads through the daemon. With PARAPHRASE_SCREENSHOTS=<dir> it saves the window.

#include "app_daemon.h"
#include "daemon_client.h"
#include "ui/common/chip_button.h"
#include "ui/setup/model_card.h"
#include "ui/setup/setup_window.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;
using paraphrase::app::ChipButton;
using paraphrase::app::ModelCard;
using paraphrase::app::SetupWindow;
using paraphrase::client::DaemonClient;
using paraphrase::testing::AppDaemon;

namespace {

constexpr int kTimeoutMs = AppDaemon::kTimeoutMs;

bool installed(DaemonClient& client, const QString& id)
{
    const QJsonArray models =
        QJsonDocument::fromJson(client.call(u"ListModels"_s, {}).arguments().value(0).toString().toUtf8())
            .array();
    for (const auto& m : models) {
        if (m.toObject().value(u"id"_s).toString() == id) {
            return m.toObject().value(u"installed"_s).toBool();
        }
    }
    return false;
}

} // namespace

class TestSetupWindow : public QObject
{
    Q_OBJECT

private:
    AppDaemon m_daemon;

private Q_SLOTS:
    void cleanup() { m_daemon.stop(); }

    void offersFastAndQualityAndWaitsForAClick()
    {
        QVERIFY(m_daemon.start({}));
        DaemonClient client;
        QVERIFY(client.connectToDaemon().isEmpty());
        SetupWindow w(client);
        w.start();
        QCOMPARE(w.step(), SetupWindow::Model);
        const auto cards = w.findChildren<ModelCard*>();
        QCOMPARE(cards.size(), 2);
        QVERIFY(w.findChild<QCheckBox*>(u"tick-fast-model"_s)->isChecked());     // recommended
        QVERIFY(!w.findChild<QCheckBox*>(u"tick-quality-model"_s)->isChecked()); // optional
        // Nothing downloads before the user clicks Download.
        QTest::qWait(200);
        QVERIFY(!installed(client, u"fast-model"_s));
        QVERIFY(!w.findChild<ChipButton*>(u"next"_s)->isEnabled());
    }

    void downloadsOnClickAndContinues()
    {
        QVERIFY(m_daemon.start({}));
        DaemonClient client;
        QVERIFY(client.connectToDaemon().isEmpty());
        SetupWindow w(client);
        w.start();
        auto* download = w.findChild<QPushButton*>(u"download-fast-model"_s);
        QVERIFY(download != nullptr);
        QCOMPARE(download->text(), u"Download"_s);
        download->click();
        auto* next = w.findChild<ChipButton*>(u"next"_s);
        QTRY_VERIFY_WITH_TIMEOUT(next->isEnabled(), kTimeoutMs);
        QVERIFY(installed(client, u"fast-model"_s));
        QCOMPARE(next->label(), u"Continue"_s);
        next->click();
        QCOMPARE(w.step(), SetupWindow::Try);

        // Try it: one proofread through the daemon.
        {
            const QMutexLocker lock(&m_daemon.state->lock);
            m_daemon.state->answers = {u"They're going to the market tomorrow, don't be late."_s};
        }
        w.findChild<QPushButton*>(u"tryIt"_s)->click();
        const auto* result = w.findChild<QLabel*>(u"tryResult"_s);
        QTRY_COMPARE_WITH_TIMEOUT(result->text(), u"They're going to the market tomorrow, don't be late."_s,
                                  kTimeoutMs);
        const QSignalSpy finished(&w, &SetupWindow::finished);
        next->click();
        QCOMPARE(finished.size(), 1);
    }

    void qualityTooLargeCannotBeTicked()
    {
        QVERIFY(m_daemon.start({}, 7.6));
        DaemonClient client;
        QVERIFY(client.connectToDaemon().isEmpty());
        SetupWindow w(client);
        w.start();
        const auto* quality = w.findChild<QCheckBox*>(u"tick-quality-model"_s);
        QVERIFY(quality != nullptr);
        QVERIFY(!quality->isEnabled());
        QVERIFY(!quality->isChecked());
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
            QVERIFY(m_daemon.start({}, 16));
            DaemonClient client;
            QVERIFY(client.connectToDaemon().isEmpty());
            SetupWindow w(client);
            w.resize(820, 700);
            w.show();
            w.start();
            QTest::qWait(50); // widgets added to a visible window show on the next event loop pass
            QCoreApplication::processEvents(); // queued shows land first
            QVERIFY(w.grab().save(dir + u"/setup-model-%1.png"_s.arg(QLatin1StringView(theme))));
            w.findChild<QPushButton*>(u"download-fast-model"_s)->click();
            QTRY_VERIFY_WITH_TIMEOUT(w.findChild<ChipButton*>(u"next"_s)->isEnabled(), kTimeoutMs);
            QTest::qWait(50);
            QCoreApplication::processEvents(); // queued shows land first
            QVERIFY(w.grab().save(dir + u"/setup-model-ready-%1.png"_s.arg(QLatin1StringView(theme))));
            m_daemon.stop();
        }
        qunsetenv("PARAPHRASE_THEME");
    }
};

QTEST_MAIN(TestSetupWindow)
#include "tst_setup_window.moc"
