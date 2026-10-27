#include "qt_clipboard_access.h"

#include <QClipboard>
#include <QGuiApplication>

namespace paraphrase::app {

void QtClipboardAccess::setText(const QString& text)
{
    QGuiApplication::clipboard()->setText(text);
}

} // namespace paraphrase::app
