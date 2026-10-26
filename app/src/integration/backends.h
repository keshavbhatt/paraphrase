#pragma once

#include <QKeySequence>
#include <QObject>
#include <QString>

#include <cstdint>
#include <optional>

namespace paraphrase::app {

/// Text the popup works on, and where it came from (PLAN 6).
struct CapturedText
{
    QString text;
    QString origin;  ///< "selection" or "clipboard": the popup says which
    QString appName; ///< the app the text came from, for "from Telegram"; may be empty
    QString window;  ///< an id of the window that had focus, to check before typing into it
};

/// How a result went back.
enum class InsertOutcome : std::uint8_t
{
    Typed,       ///< pasted into the remembered window
    OnClipboard, ///< left on the clipboard: the user presses Ctrl+V (the popup says so)
    Failed,
};

/// Whether Paraphrase may type a result back into another app (setup-desktop.html).
enum class TypingPermission : std::uint8_t
{
    NotNeeded, ///< the session lets it type as it is (X11)
    NotAsked,  ///< the desktop asks once, and has not yet
    Allowed,
    Refused,     ///< the user said no: results go on the clipboard
    Unavailable, ///< this desktop offers no way to type: results go on the clipboard
};

/// What the app can do on this desktop, in plain words, for Settings and the notice.
struct BackendStatus
{
    QString kind; ///< shortcut, capture or insert
    QString name; ///< e.g. "GlobalShortcuts portal"
    bool works = false;
    QString detail; ///< what the user can do when it does not
};

/// Reads the text the user means: the selection where the session allows it, else the
/// clipboard (PLAN 6). Implementations: clipboard (always), X11 PRIMARY, Wayland data-control.
class CaptureBackend
{
public:
    CaptureBackend() = default;
    virtual ~CaptureBackend() = default;
    CaptureBackend(const CaptureBackend&) = delete;
    CaptureBackend& operator=(const CaptureBackend&) = delete;
    CaptureBackend(CaptureBackend&&) = delete;
    CaptureBackend& operator=(CaptureBackend&&) = delete;

    [[nodiscard]] virtual QString name() const = 0;
    [[nodiscard]] virtual bool available() const = 0;
    /// The text now, or nothing when there is none. Never logged (CLAUDE.md).
    [[nodiscard]] virtual std::optional<CapturedText> capture() = 0;

    /// Text a result was just put in place of. It is gone from the app, so a selection that
    /// still reads as it is stale: some apps (a browser's canvas editor, a select-all from
    /// the keyboard) never update the highlighted text, and the popup opened on the old text.
    /// `clipboard` is what the clipboard holds after it (the result): after a stale selection
    /// the clipboard counts only once the user copies something new.
    void setReplaced(const QString& text, const QString& clipboard)
    {
        m_replaced = text.trimmed();
        m_clipboardThen = clipboard.trimmed();
    }
    /// The last capture passed over that stale text.
    [[nodiscard]] bool skippedReplaced() const { return m_skipped; }

protected:
    /// For the backends: call first in capture().
    void beginCapture() { m_skipped = false; }
    /// For the backends: true, and noted, for a selection that is only the replaced text.
    [[nodiscard]] bool isReplaced(const QString& selection)
    {
        const bool gone = !m_replaced.isEmpty() && selection.trimmed() == m_replaced;
        m_skipped = m_skipped || gone;
        return gone;
    }
    /// For the backends: the same for the clipboard, which after a stale selection must also
    /// have changed since the result went in.
    [[nodiscard]] bool isStaleClipboard(const QString& copied)
    {
        const bool unchanged = m_skipped && copied.trimmed() == m_clipboardThen;
        return isReplaced(copied) || unchanged;
    }

private:
    QString m_replaced;
    QString m_clipboardThen;
    bool m_skipped = false;
};

/// Puts a result back where the text came from, only after an explicit user action
/// (Replace, Insert reply). Implementations: clipboard (always), X11 XTest, RemoteDesktop portal.
class InsertBackend
{
public:
    InsertBackend() = default;
    virtual ~InsertBackend() = default;
    InsertBackend(const InsertBackend&) = delete;
    InsertBackend& operator=(const InsertBackend&) = delete;
    InsertBackend(InsertBackend&&) = delete;
    InsertBackend& operator=(InsertBackend&&) = delete;

    [[nodiscard]] virtual QString name() const = 0;
    [[nodiscard]] virtual bool available() const = 0;
    /// Never types into a window the user did not choose: when `target.window` no longer has
    /// focus, implementations leave the text on the clipboard instead (PLAN 6).
    [[nodiscard]] virtual InsertOutcome insert(const QString& text, const CapturedText& target) = 0;
    [[nodiscard]] virtual TypingPermission typingPermission() const { return TypingPermission::Unavailable; }
    /// Asks the desktop now, when it asks at all (its own dialog; waits for the answer).
    virtual TypingPermission askTyping() { return typingPermission(); }
};

/// The global shortcut that opens the popup. Implementations: X11 key grab, the
/// GlobalShortcuts portal, and "bind this command" (always: the user binds `paraphrase-app
/// --popup` in the desktop's settings).
class ShortcutBackend : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ShortcutBackend)

public:
    explicit ShortcutBackend(QObject* parent = nullptr)
        : QObject(parent)
    {}
    ~ShortcutBackend() override = default;

    [[nodiscard]] virtual QString name() const = 0;
    [[nodiscard]] virtual bool available() const = 0;
    /// Asks for the shortcut; false when the desktop refused or does not offer it.
    virtual bool bind(const QKeySequence& keys) = 0;
    /// What the user sees in Settings: the shortcut, or how to set one by hand.
    [[nodiscard]] virtual QString description() const = 0;
    /// The keys that open the popup, as the desktop shows them ("Ctrl+Alt+Space"), or "".
    [[nodiscard]] virtual QString keys() const { return {}; }

Q_SIGNALS:
    void activated();
    /// keys() changed: the desktop bound the shortcut, or the user changed it in its settings.
    void keysChanged();
};

} // namespace paraphrase::app
