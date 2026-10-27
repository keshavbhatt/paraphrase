// The first-run window (setup-model.html) against a real daemon service with a fake engine:
// Fast is preselected and Quality optional, nothing downloads before a click, the download
// panel follows the daemon's progress, Continue waits for an installed model, and "Try it"
// proofreads through the daemon. With PARAPHRASE_SCREENSHOTS=<dir> it saves the window.

#include "app_daemon.h"
#include "daemon_client.h"
#include "shell/app_flow.h"
#include "ui/common/chip_button.h"
#include "ui/core/desktop_support.h"
#include "ui/main/main_window.h"
#include "ui/setup/model_card.h"
#include "ui/setup/setup_window.h"

#include <QApplication>
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

#include <algorithm>

using namespace Qt::StringLiterals;
using paraphrase::app::AppFlow;
using paraphrase::app::ChipButton;
using paraphrase::app::DesktopSupport;
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
        QCOMPARE(w.step(), SetupWindow::Shortcut); // over any app first (setup-desktop.html)
        w.findChild<ChipButton*>(u"next"_s)->click();
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
        w.findChild<QPushButton*>(u"skip"_s)->click(); // both are optional
        QCOMPARE(w.step(), SetupWindow::Model);
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

    void opensTheMainWindowWhenSetupIsDone()
    {
        // "Open Paraphrase" on the last step: setup closes and the main window opens, so the app
        // never runs out of windows (it quit here once, in the snap).
        QVERIFY(m_daemon.start({}));
        {
            const QMutexLocker lock(&m_daemon.state->lock);
            m_daemon.state->answers = {u"They're going to the market tomorrow."_s};
        }
        DaemonClient client;
        AppFlow flow(client);
        flow.start();
        SetupWindow* w = nullptr;
        for (QWidget* top : QApplication::topLevelWidgets()) {
            if (auto* s = qobject_cast<SetupWindow*>(top); s != nullptr && s->isVisible()) {
                w = s;
            }
        }
        QVERIFY(w != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(w->findChild<QPushButton*>(u"skip"_s) != nullptr, kTimeoutMs);
        w->findChild<QPushButton*>(u"skip"_s)->click();
        QTRY_VERIFY_WITH_TIMEOUT(w->findChild<QPushButton*>(u"download-fast-model"_s) != nullptr, kTimeoutMs);
        w->findChild<QPushButton*>(u"download-fast-model"_s)->click();
        auto* next = w->findChild<ChipButton*>(u"next"_s);
        QTRY_VERIFY_WITH_TIMEOUT(next->isEnabled(), kTimeoutMs);
        next->click();
        QCOMPARE(w->step(), SetupWindow::Try);
        next->click();
        QVERIFY(flow.mainWindow().isVisible());
        QVERIFY(!w->isVisible());
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

    void asksToTypeOnlyOnAllow()
    {
        // setup-desktop.html: the desktop's dialog opens on Allow, not before; its answer shows.
        QVERIFY(m_daemon.start({}));
        DaemonClient client;
        QVERIFY(client.connectToDaemon().isEmpty());
        SetupWindow w(client);
        int asked = 0;
        DesktopSupport support;
        support.globalShortcut = true;
        support.shortcut = u"Ctrl+Alt+Space"_s;
        support.typing = DesktopSupport::Typing::NotAsked;
        support.askTyping = [&asked] {
            ++asked;
            return DesktopSupport::Typing::Allowed;
        };
        w.setDesktopSupport(support);
        w.start();
        QCOMPARE(asked, 0);
        auto* allow = w.findChild<QPushButton*>(u"allowTyping"_s);
        QVERIFY(allow != nullptr);
        allow->click();
        QTRY_COMPARE(asked, 1);
        QTRY_VERIFY(allow->isHidden());
        bool allowed = false;
        for (const QLabel* l : w.findChildren<QLabel*>()) {
            allowed = allowed || l->text().contains(u"Allowed"_s);
        }
        QVERIFY(allowed);
    }

    void letsTheUserChooseTheKeys()
    {
        // setup-desktop.html, "Shortcut", every state: no keys offers Choose keys (b), keys that
        // are set offer Change keys (a), and the row follows each new answer from the desktop.
        QVERIFY(m_daemon.start({}));
        DaemonClient client;
        QVERIFY(client.connectToDaemon().isEmpty());
        SetupWindow w(client);
        int chosen = 0;
        DesktopSupport support;
        support.globalShortcut = true;
        support.canChangeKeys = true;
        support.chooseKeys = [&chosen] { ++chosen; };
        w.setDesktopSupport(support);
        w.resize(820, 700);
        w.show();
        w.start();
        QCoreApplication::processEvents();
        auto* choose = w.findChild<QPushButton*>(u"chooseKeys"_s);
        QVERIFY(choose != nullptr);
        const auto hasLabel = [&w](const QString& text) {
            const auto labels = w.findChildren<QLabel*>();
            return std::ranges::any_of(labels, [&text](const QLabel* l) { return l->text() == text; });
        };
        QVERIFY(hasLabel(u"No keys"_s));
        QVERIFY(!hasLabel(u"By hand"_s));
        const QString dir = qEnvironmentVariable("PARAPHRASE_SCREENSHOTS");
        if (!dir.isEmpty()) {
            QTest::qWait(50);
            QVERIFY(w.grab().save(dir + u"/setup-desktop-nokeys.png"_s));
        }
        choose->click();
        QCOMPARE(chosen, 1);

        support.shortcut = u"Ctrl+Shift+P"_s; // the desktop's answer: ShortcutsChanged
        w.setDesktopSupport(support);
        QCoreApplication::processEvents();
        QTRY_VERIFY(w.findChild<QPushButton*>(u"chooseKeys"_s) == nullptr);
        auto* change = w.findChild<QPushButton*>(u"changeKeys"_s);
        QVERIFY(change != nullptr);
        QVERIFY(hasLabel(u"Shift"_s));
        if (!dir.isEmpty()) {
            QTest::qWait(50);
            QVERIFY(w.grab().save(dir + u"/setup-desktop-changekeys.png"_s));
        }
        change->click();
        QCOMPARE(chosen, 2);
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
            DesktopSupport support;
            support.globalShortcut = true;
            support.shortcut = u"Ctrl+Alt+Space"_s;
            support.typing = DesktopSupport::Typing::NotAsked;
            support.askTyping = [] { return DesktopSupport::Typing::Allowed; };
            w.setDesktopSupport(support);
            w.resize(820, 700);
            w.show();
            w.start();
            QTest::qWait(50); // widgets added to a visible window show on the next event loop pass
            QCoreApplication::processEvents(); // queued shows land first
            QVERIFY(w.grab().save(dir + u"/setup-desktop-%1.png"_s.arg(QLatin1StringView(theme))));
            w.findChild<ChipButton*>(u"next"_s)->click();
            QTest::qWait(50);
            QCoreApplication::processEvents();
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
