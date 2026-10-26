#pragma once

#include "backends.h"

namespace paraphrase::app {

/// The selection where Qt can read it (X11, and Wayland compositors with the primary-selection
/// protocol while a Paraphrase window has focus), else the clipboard. Works everywhere.
class ClipboardCapture final : public CaptureBackend
{
public:
    [[nodiscard]] QString name() const override;
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::optional<CapturedText> capture() override;
    [[nodiscard]] bool needsFocus() const override;
};

/// Leaves the result on the clipboard; the popup tells the user to press Ctrl+V. Works
/// everywhere, types nothing.
class ClipboardInsert final : public InsertBackend
{
public:
    [[nodiscard]] QString name() const override;
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] InsertOutcome insert(const QString& text, const CapturedText& target) override;
};

/// "Bind this command": the desktop runs `paraphrase-app --popup`, which asks the running
/// app to open the popup over D-Bus. Works on every desktop that has custom shortcuts.
class CommandShortcut final : public ShortcutBackend
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(CommandShortcut)

public:
    explicit CommandShortcut(QObject* parent = nullptr)
        : ShortcutBackend(parent)
    {}
    ~CommandShortcut() override = default;

    [[nodiscard]] QString name() const override;
    [[nodiscard]] bool available() const override { return true; }
    bool bind(const QKeySequence& /*keys*/) override { return true; }
    [[nodiscard]] QString description() const override;
};

} // namespace paraphrase::app
