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
};

} // namespace paraphrase::app
