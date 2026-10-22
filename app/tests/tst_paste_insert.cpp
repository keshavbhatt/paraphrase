// Putting a result back (PLAN 6) with fakes for the window, the clipboard and the keys: it
// types only into the window the text came from, leaves the result on the clipboard
// otherwise, and puts the user's clipboard back after a paste.

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
    [[nodiscard]] ClipboardContent save() override { return content; }
    void setText(const QString& text) override { content = {{u"text/plain"_s, text.toUtf8()}}; }
    void restore(const ClipboardContent& saved) override { content = saved; }
    [[nodiscard]] QByteArray text() const { return content.value(0).second; }
};

class FakeKeys final : public PasteKeys
{
public:
    bool works = true;
    int pastes = 0;
    QByteArray clipboardAtPaste;
    FakeClipboard* clipboard = nullptr;
    [[nodiscard]] QString name() const override { return u"fake keys"_s; }
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] bool sendPaste() override
    {
        ++pastes;
        clipboardAtPaste = clipboard->text();
        return works;
    }
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

    void pastesIntoTheSameWindowAndRestoresTheClipboard()
    {
        QCOMPARE(m_insert->insert(u"Mia doesn't know"_s, fromTelegram()), InsertOutcome::Typed);
        QCOMPARE(m_keys->pastes, 1);
        QCOMPARE(m_keys->clipboardAtPaste, QByteArray("Mia doesn't know"));
        // A moment later the user's clipboard is back, every format of it.
        QTRY_COMPARE_WITH_TIMEOUT(m_clipboard->content.size(), 2, PasteInsert::kRestoreDelayMs * 3);
        QCOMPARE(m_clipboard->text(), QByteArray("what the user copied"));
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
        QTest::qWait(PasteInsert::kRestoreDelayMs * 2);
        QCOMPARE(m_clipboard->text(), QByteArray("Mia doesn't know")); // not replaced by the old copy
    }

    void finishesAPendingRestoreFirst()
    {
        QCOMPARE(m_insert->insert(u"first"_s, fromTelegram()), InsertOutcome::Typed);
        QCOMPARE(m_insert->insert(u"second"_s, fromTelegram()), InsertOutcome::Typed);
        QCOMPARE(m_keys->clipboardAtPaste, QByteArray("second"));
        m_insert->restoreNow();
        QCOMPARE(m_clipboard->text(), QByteArray("what the user copied")); // not "first"
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
