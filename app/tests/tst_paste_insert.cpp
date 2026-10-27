// Putting a result back (PLAN 6) with fakes for the window, the clipboard and the keys: it
// types only into the window the text came from and leaves the result on the clipboard,
// with the paste keys every keyboard layout has.

#include "integration/paste_insert.h"
#include "integration/portal_shortcut.h"

#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;
using namespace paraphrase::app;

namespace {

class FakeTracker final : public WindowTracker
{
public:
    QString active = u"telegram-1"_s;
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] QString activeWindow() const override { return active; }
    [[nodiscard]] QString activeApp() const override { return u"Telegram"_s; }
    [[nodiscard]] bool waitUntilActive(const QString& window, int /*timeoutMs*/) override
    {
        return window == active;
    }
};

class FakeClipboard final : public ClipboardAccess
{
public:
    ClipboardContent content{{u"text/plain"_s, "what the user copied"}, {u"image/png"_s, "png bytes"}};
    void setText(const QString& text) override { content = {{u"text/plain"_s, text.toUtf8()}}; }
    [[nodiscard]] QByteArray text() const { return content.value(0).second; }
};

class FakeKeys final : public PasteKeys
{
public:
    bool works = true;
    int pastes = 0;
    PasteChord chord = PasteChord::ShiftInsert;
    QByteArray clipboardAtPaste;
    FakeClipboard* clipboard = nullptr;
    [[nodiscard]] QString name() const override { return u"fake keys"_s; }
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] bool sendPaste(PasteChord sent) override
    {
        ++pastes;
        chord = sent;
        clipboardAtPaste = clipboard->text();
        return works;
    }
};

class FakeShortcut final : public ShortcutBackend
{
public:
    [[nodiscard]] QString name() const override { return u"fake shortcut"_s; }
    [[nodiscard]] bool available() const override { return true; }
    bool bind(const QKeySequence& /*keys*/) override { return true; }
    [[nodiscard]] QString description() const override { return {}; }
    void press() { trigger(); }
};

CapturedText fromTelegram()
{
    return {
        .text = u"Mia dont know"_s,
        .origin = u"selection"_s,
        .appName = u"Telegram"_s,
        .window = u"telegram-1"_s,
    };
}

} // namespace

class TestPasteInsert : public QObject
{
    Q_OBJECT

private:
    std::shared_ptr<FakeTracker> m_tracker;
    std::shared_ptr<FakeClipboard> m_clipboard;
    std::shared_ptr<FakeKeys> m_keys;
    std::unique_ptr<PasteInsert> m_insert;

private Q_SLOTS:
    void init()
    {
        m_tracker = std::make_shared<FakeTracker>();
        m_clipboard = std::make_shared<FakeClipboard>();
        m_keys = std::make_shared<FakeKeys>();
        m_keys->clipboard = m_clipboard.get();
        m_insert = std::make_unique<PasteInsert>(u"fake"_s, m_tracker, m_clipboard, m_keys);
    }

    void pastesIntoTheSameWindowAndKeepsTheResultOnTheClipboard()
    {
        QCOMPARE(m_insert->insert(u"Mia doesn't know"_s, fromTelegram()), InsertOutcome::Typed);
        QCOMPARE(m_keys->pastes, 1);
        QCOMPARE(m_keys->clipboardAtPaste, QByteArray("Mia doesn't know"));
        // It stays: a paste with no text field in focus lands nowhere (owner, 2026-10-26).
        QTest::qWait(1000);
        QCOMPARE(m_clipboard->text(), QByteArray("Mia doesn't know"));
    }

    void neverTypesIntoAnotherWindow()
    {
        m_tracker->active = u"browser-7"_s; // the user moved on
        QCOMPARE(m_insert->insert(u"Mia doesn't know"_s, fromTelegram()), InsertOutcome::OnClipboard);
        QCOMPARE(m_keys->pastes, 0);
        QCOMPARE(m_clipboard->text(), QByteArray("Mia doesn't know")); // ready for Ctrl+V
    }

    void needsToKnowTheWindow()
    {
        CapturedText unknown = fromTelegram();
        unknown.window.clear(); // the clipboard fallback cannot tell
        QCOMPARE(m_insert->insert(u"x"_s, unknown), InsertOutcome::OnClipboard);
        QCOMPARE(m_keys->pastes, 0);
    }

    void keepsTheResultWhenTheKeysFail()
    {
        m_keys->works = false; // no permission, the session ended
        QCOMPARE(m_insert->insert(u"Mia doesn't know"_s, fromTelegram()), InsertOutcome::OnClipboard);
        QCOMPARE(m_clipboard->text(), QByteArray("Mia doesn't know"));
    }

    void pastesWithKeysEveryLayoutHas()
    {
        // Shift+Insert: a Russian or Greek layout may have no "v" for Ctrl+V (dabri #105).
        QCOMPARE(m_insert->insert(u"Mia doesn't know"_s, fromTelegram()), InsertOutcome::Typed);
        QCOMPARE(m_keys->chord, PasteChord::ShiftInsert);
        QCOMPARE(chordKeysyms(PasteChord::ShiftInsert), (std::array<std::uint32_t, 3>{0xffe1, 0xff63, 0}));
    }

    void pastesTheClipboardInTerminals()
    {
        // A terminal pastes the selection with Shift+Insert, the very text the user selected.
        CapturedText fromKonsole = fromTelegram();
        fromKonsole.appName = u"org.kde.konsole"_s;
        QCOMPARE(m_insert->insert(u"git status"_s, fromKonsole), InsertOutcome::Typed);
        QCOMPARE(m_keys->chord, PasteChord::CtrlShiftV);
        QCOMPARE(chordKeysyms(PasteChord::CtrlShiftV), (std::array<std::uint32_t, 3>{0xffe3, 0xffe1, 0x76}));
        QCOMPARE(pasteChordFor(u"Gnome-terminal"_s), PasteChord::CtrlShiftV); // X11 WM_CLASS
        QCOMPARE(pasteChordFor(u"kitty"_s), PasteChord::CtrlShiftV);
        QCOMPARE(pasteChordFor(u"XTerm"_s), PasteChord::ShiftInsert); // its only paste
        QCOMPARE(pasteChordFor(QString()), PasteChord::ShiftInsert);
    }

    void opensOncePerHoldOfTheShortcut()
    {
        // GNOME repeats Activated while the keys are held, X11 repeats the key press.
        FakeShortcut shortcut;
        const QSignalSpy activated(&shortcut, &ShortcutBackend::activated);
        shortcut.press();
        for (int i = 0; i < 10; ++i) { // held for half a second
            QTest::qWait(50);
            shortcut.press();
        }
        QCOMPARE(activated.count(), 1);
        QTest::qWait(ShortcutBackend::kRepeatMs + 100); // let go, then a new press
        shortcut.press();
        QCOMPARE(activated.count(), 2);
    }

    void writesTriggersAsThePortalExpects()
    {
        QCOMPARE(PortalShortcut::preferredTrigger(QKeySequence(u"Ctrl+Alt+Space"_s)), u"CTRL+ALT+space"_s);
        QCOMPARE(PortalShortcut::preferredTrigger(QKeySequence(u"Meta+Shift+P"_s)), u"SHIFT+LOGO+p"_s);
        QCOMPARE(PortalShortcut::preferredTrigger(QKeySequence(u"Ctrl+F2"_s)), u"CTRL+F2"_s);
    }
};

QTEST_GUILESS_MAIN(TestPasteInsert)
#include "tst_paste_insert.moc"
