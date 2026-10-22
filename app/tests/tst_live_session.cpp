// Read-only checks on the user's own KDE Plasma Wayland session, run by hand with
// PARAPHRASE_TEST_LIVE=1: nothing is typed, no clipboard is read or changed, no shortcut is
// bound and no dialog opens. It checks that the session offers what the backends need.

#include "integration/data_control_client.h"
#include "integration/kwin_window_tracker.h"
#include "integration/portal.h"
#include "integration/portal_paste_keys.h"
#include "integration/portal_shortcut.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QGuiApplication>
#include <QTest>

using namespace Qt::StringLiterals;
using namespace paraphrase::app;

class TestLiveSession : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        if (qEnvironmentVariable("PARAPHRASE_TEST_LIVE") != u"1"_s ||
            QGuiApplication::platformName() != u"wayland"_s) {
            QSKIP("by hand only: PARAPHRASE_TEST_LIVE=1 on a Wayland session");
        }
    }

    void tracksTheActiveWindowThroughKWin()
    {
        const KWinWindowTracker tracker;
        QVERIFY2(tracker.available(), "KWin scripting did not report the active window");
        QVERIFY(!tracker.activeWindow().isEmpty());
        // The script is unloaded when the tracker goes; nothing is left in KWin.
    }

    void offersThePortals()
    {
        QVERIFY(PortalShortcut().available());
        QVERIFY(PortalPasteKeys().available());
    }

    void opensAndClosesAShortcutSession()
    {
        // CreateSession asks nothing of the user; binding (which may ask) is not done here.
        portal::registerApp();
        const auto created =
            portal::request(u"org.freedesktop.portal.GlobalShortcuts"_s, u"CreateSession"_s,
                            {QVariantMap{{u"session_handle_token"_s, portal::token()}}}, 10'000);
        QVERIFY(created.has_value());
        const QString session = created.value_or(QVariantMap()).value(u"session_handle"_s).toString();
        QVERIFY(!session.isEmpty());
        const QDBusMessage close = QDBusMessage::createMethodCall(
            QString::fromLatin1(portal::kService), session, u"org.freedesktop.portal.Session"_s, u"Close"_s);
        QCOMPARE(QDBusConnection::sessionBus().call(close).type(), QDBusMessage::ReplyMessage);
    }

    void connectsToDataControl()
    {
        // Connects and lists formats only: the clipboard's content is not read.
        const auto client = DataControlClient::connectToCompositor();
        QVERIFY2(client != nullptr, "KWin offers no ext-data-control-v1");
    }
};

QTEST_MAIN(TestLiveSession)
#include "tst_live_session.moc"
