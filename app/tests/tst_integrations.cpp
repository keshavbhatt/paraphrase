// The ranked backends (PLAN 6): the first available one of each kind is used, the clipboard
// and "bind this command" always work, and the status lists every backend for Settings.

#include "daemon_client.h"
#include "integration/clipboard_backends.h"
#include "integration/integrations.h"
#include "integration/kwin_window_tracker.h"
#include "integration/launch_command.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;
using namespace paraphrase::app;

namespace {

class FakeCapture final : public CaptureBackend
{
public:
    explicit FakeCapture(bool works)
        : m_works(works)
    {}
    [[nodiscard]] QString name() const override { return u"fake capture"_s; }
    [[nodiscard]] bool available() const override { return m_works; }
    [[nodiscard]] std::optional<CapturedText> capture() override
    {
        return CapturedText{.text = u"from fake"_s, .origin = u"selection"_s, .appName = {}, .window = {}};
    }

private:
    bool m_works;
};

class FakeShortcut final : public ShortcutBackend
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FakeShortcut)

public:
    FakeShortcut(bool works, QObject* parent)
        : ShortcutBackend(parent)
        , m_works(works)
    {}
    ~FakeShortcut() override = default;
    [[nodiscard]] QString name() const override { return u"fake shortcut"_s; }
    [[nodiscard]] bool available() const override { return m_works; }
    bool bind(const QKeySequence& /*keys*/) override { return m_works; }
    [[nodiscard]] QString description() const override { return u"Ctrl+Alt+Space"_s; }

private:
    bool m_works;
};

} // namespace

class TestIntegrations : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void namesTheCommandForEachPackage()
    {
        // The shortcut to bind and the login entry start the app the way its package does.
        QCOMPARE(desktopFileName(), u"com.ktechpit.paraphrase"_s);
        qputenv("SNAP_NAME", "paraphrase");
        QCOMPARE(launchLine({u"--popup"_s}), u"paraphrase --popup"_s);
        // snapd installs the desktop file as <snap>_<app>; the window's app id must match it.
        QCOMPARE(desktopFileName(), u"paraphrase_paraphrase"_s);
        qunsetenv("SNAP_NAME");
        qputenv("FLATPAK_ID", "com.ktechpit.paraphrase");
        QCOMPARE(launchLine({u"--hidden"_s}), u"flatpak run com.ktechpit.paraphrase --hidden"_s);
        // KWin reads the window tracker's script from the one runtime folder the host shares.
        const QTemporaryDir runtime;
        const QByteArray realRuntime = qgetenv("XDG_RUNTIME_DIR");
        qputenv("XDG_RUNTIME_DIR", runtime.path().toUtf8());
        QCOMPARE(paraphrase::app::KWinWindowTracker::scriptPath(),
                 runtime.path() + u"/app/com.ktechpit.paraphrase/paraphrase-window-tracker.js"_s);
        qputenv("XDG_RUNTIME_DIR", realRuntime);
        // The bus filter passes the daemon's answers only to clients owning a name under the app id.
        QCOMPARE(paraphrase::client::DaemonClient::sandboxName(),
                 u"com.ktechpit.paraphrase.Client.p%1"_s.arg(QCoreApplication::applicationPid()));
        qunsetenv("FLATPAK_ID");
        QVERIFY(paraphrase::client::DaemonClient::sandboxName().isEmpty());
        qputenv("APPIMAGE", "/home/a b/Paraphrase.AppImage");
        QCOMPARE(launchLine({u"--popup"_s}), u"\"/home/a b/Paraphrase.AppImage\" --popup"_s);
        qunsetenv("APPIMAGE");
    }

    void usesTheFirstBackendThatWorks()
    {
        std::vector<std::unique_ptr<CaptureBackend>> capture;
        capture.push_back(std::make_unique<FakeCapture>(false));
        capture.push_back(std::make_unique<FakeCapture>(true));
        capture.push_back(std::make_unique<ClipboardCapture>());
        std::vector<std::unique_ptr<InsertBackend>> insert;
        insert.push_back(std::make_unique<ClipboardInsert>());
        QObject owner;
        const Integrations i(std::move(capture), std::move(insert),
                             {new FakeShortcut(true, &owner), new CommandShortcut(&owner)});
        const auto got = i.capture().capture();
        QVERIFY(got.has_value());
        QCOMPARE(got.value_or(CapturedText{}).text, u"from fake"_s);
        QVERIFY(i.hasGlobalShortcut());
        QCOMPARE(i.status().size(), 6);
    }

    void fallsBackToTheClipboardAndACommand()
    {
        QObject owner;
        const Integrations i = Integrations::forThisSession(&owner);
        QVERIFY(!i.hasGlobalShortcut());
        QVERIFY(i.shortcut().description().endsWith(u" --popup"_s));
        QGuiApplication::clipboard()->setText(u"copied text"_s);
        const CapturedText got = i.capture().capture().value_or(CapturedText{});
        QVERIFY(!got.text.isEmpty());
        // Inserting never types here: it leaves the result on the clipboard.
        QCOMPARE(i.insert().insert(u"the result"_s, got), InsertOutcome::OnClipboard);
        QCOMPARE(QGuiApplication::clipboard()->text(), u"the result"_s);
    }

    void neverCapturesTextThatWasReplaced()
    {
        // A result went in place of this text: a selection that still reads as it is stale (a
        // browser canvas kept the old highlighted text), and so is the clipboard the paste put
        // back. Only something copied since counts.
        ClipboardCapture capture;
        QGuiApplication::clipboard()->setText(u"Single appointment: we printing?"_s);
        capture.setReplaced(u"Single appointment: we printing?"_s, u"an older copy"_s);
        QVERIFY(!capture.capture().has_value());
        QVERIFY(capture.skippedReplaced()); // the popup says to copy the new text
        QGuiApplication::clipboard()->setText(u"Series appointment: we printing?"_s);
        QCOMPARE(capture.capture().value_or(CapturedText{}).text, u"Series appointment: we printing?"_s);
        QVERIFY(!capture.skippedReplaced());
    }
};

QTEST_MAIN(TestIntegrations)
#include "tst_integrations.moc"
