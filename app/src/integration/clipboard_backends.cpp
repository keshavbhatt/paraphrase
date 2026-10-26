#include "clipboard_backends.h"

#include "integration/launch_command.h"

#include <QClipboard>
#include <QGuiApplication>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

QString ClipboardCapture::name() const
{
    return QObject::tr("Selection or clipboard");
}

std::optional<CapturedText> ClipboardCapture::capture()
{
    beginCapture();
    const QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard->supportsSelection()) {
        if (const QString selected = clipboard->text(QClipboard::Selection);
            !selected.trimmed().isEmpty() && !isReplaced(selected)) {
            return CapturedText{.text = selected, .origin = u"selection"_s, .appName = {}, .window = {}};
        }
    }
    const QString copied = clipboard->text(QClipboard::Clipboard);
    if (copied.trimmed().isEmpty() || isStaleClipboard(copied)) {
        return std::nullopt;
    }
    return CapturedText{.text = copied, .origin = u"clipboard"_s, .appName = {}, .window = {}};
}

bool ClipboardCapture::needsFocus() const
{
    // Wayland sends the clipboard to the window with keyboard focus only; X11 answers anyone.
    return QGuiApplication::platformName().startsWith(u"wayland"_s);
}

QString ClipboardInsert::name() const
{
    return QObject::tr("Clipboard");
}

InsertOutcome ClipboardInsert::insert(const QString& text, const CapturedText& /*target*/)
{
    QGuiApplication::clipboard()->setText(text);
    return InsertOutcome::OnClipboard;
}

QString CommandShortcut::name() const
{
    return tr("Command bound in the desktop's settings");
}

QString CommandShortcut::description() const
{
    return tr("Add a custom shortcut in your desktop's settings that runs: %1")
        .arg(launchLine({u"--popup"_s}));
}

} // namespace paraphrase::app
