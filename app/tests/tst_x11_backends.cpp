// X11 backends on the Xwayland display of a headless KWin started by scripts/desktop-test.sh,
// never the user's session, typing only into a window this test opens: the key grab fires,
// the tracker sees the test window, and a paste lands in it with the clipboard restored.

#include "integration/paste_insert.h"
#include "integration/qt_clipboard_access.h"
#include "integration/selection_captures.h"
#include "integration/x11.h"
#include "integration/x11_backends.h"

#include <QApplication>
#include <QClipboard>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTest>

#include <array>
#include <ranges>
#include <xcb/xcb.h>
#include <xcb/xtest.h>

using namespace Qt::StringLiterals;
using namespace paraphrase::app;

class TestX11Backends : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QLineEdit> m_target;
    std::shared_ptr<X11WindowTracker> m_tracker;

private Q_SLOTS:
    void initTestCase()
    {
        if (qEnvironmentVariable("PARAPHRASE_TEST_NESTED") != u"1"_s ||
            QGuiApplication::platformName() != u"xcb"_s) {
            QSKIP(
                "run through scripts/desktop-test.sh: it starts a headless KWin with Xwayland for this test");
        }
        m_tracker = std::make_shared<X11WindowTracker>();
        m_target = std::make_unique<QLineEdit>();
        m_target->setWindowTitle(u"Paraphrase test target"_s);
        m_target->show();
        m_target->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(m_target.get()));
    }

    void tracksTheTestWindow()
    {
        QTRY_COMPARE_WITH_TIMEOUT(m_tracker->activeWindow(), QString::number(m_target->winId()), 3000);
    }

    void stepsAsideOnXwayland()
    {
        if (!x11::isXwayland(x11::connection())) {
            QSKIP("a real X server: the next tests cover the grab and XTest");
        }
        // Xwayland drops XTest input and only grabs keys for X windows: the portals do it.
        QVERIFY(!XTestPasteKeys().available());
        QVERIFY(!X11Shortcut().available());
    }

    void pastesIntoTheTestWindowAndRestoresTheClipboard()
    {
        if (x11::isXwayland(x11::connection())) {
            QSKIP(
                "needs a real X server (an X11 session or Xvfb with a window manager); Xwayland drops XTest");
        }
        QApplication::clipboard()->setText(u"what the user copied"_s);
        PasteInsert insert(u"XTest"_s, m_tracker, std::make_shared<QtClipboardAccess>(),
                           std::make_shared<XTestPasteKeys>());
        QVERIFY(insert.available());
        const CapturedText target{
            .text = {},
            .origin = u"selection"_s,
            .appName = {},
            .window = QString::number(m_target->winId()),
        };
        QCOMPARE(insert.insert(u"Mia doesn't know"_s, target), InsertOutcome::Typed);
        QTRY_COMPARE_WITH_TIMEOUT(m_target->text(), u"Mia doesn't know"_s, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(QApplication::clipboard()->text(), u"what the user copied"_s, 3000);
    }

    void firesTheShortcut()
    {
        if (x11::isXwayland(x11::connection())) {
            QSKIP(
                "needs a real X server (an X11 session or Xvfb with a window manager); Xwayland drops XTest");
        }
        X11Shortcut shortcut;
        QVERIFY(shortcut.bind(QKeySequence(u"Ctrl+Alt+F9"_s)));
        QVERIFY(shortcut.description().contains(u"F9"_s));
        const QSignalSpy activated(&shortcut, &ShortcutBackend::activated);
        // Press the keys through XTest, as a user would on the keyboard.
        xcb_connection_t* conn = x11::connection();
        const std::uint32_t root = x11::rootWindow(conn);
        const std::array<std::uint32_t, 3> keysyms = {
            0xffe3 /* Control_L */,
            0xffe9 /* Alt_L */,
            0xffc6 /* F9 */,
        };
        for (const std::uint32_t sym : keysyms) {
            xcb_test_fake_input(conn, XCB_KEY_PRESS, x11::keycode(conn, sym).value_or(0), XCB_CURRENT_TIME,
                                root, 0, 0, 0);
        }
        for (const std::uint32_t sym : std::ranges::reverse_view(keysyms)) {
            xcb_test_fake_input(conn, XCB_KEY_RELEASE, x11::keycode(conn, sym).value_or(0), XCB_CURRENT_TIME,
                                root, 0, 0, 0);
        }
        xcb_flush(conn);
        QTRY_COMPARE_WITH_TIMEOUT(activated.size(), 1, 3000);
    }
};

QTEST_MAIN(TestX11Backends)
#include "tst_x11_backends.moc"
