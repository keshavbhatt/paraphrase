#include "clipboard_backends.h"

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
    return tr("Add a custom shortcut in your desktop's settings that runs: paraphrase-app --popup");
}

} // namespace paraphrase::app
