#include "qt_clipboard_access.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>

namespace paraphrase::app {

ClipboardContent QtClipboardAccess::save()
{
    ClipboardContent saved;
    const QMimeData* data = QGuiApplication::clipboard()->mimeData();
    if (data == nullptr) {
        return saved;
    }
    qsizetype size = 0;
    for (const QString& format : data->formats()) {
        QByteArray bytes = data->data(format);
        size += bytes.size();
        if (size > kMaxSaved) {
            break; // keep what fits
        }
        saved.append({format, std::move(bytes)});
    }
    return saved;
}

void QtClipboardAccess::setText(const QString& text)
{
    QGuiApplication::clipboard()->setText(text);
}

void QtClipboardAccess::restore(const ClipboardContent& content)
{
    if (content.isEmpty()) {
        QGuiApplication::clipboard()->clear();
        return;
    }
    auto* data = new QMimeData;
    for (const auto& [format, bytes] : content) {
        data->setData(format, bytes);
    }
    QGuiApplication::clipboard()->setMimeData(data); // the clipboard takes ownership
}

} // namespace paraphrase::app
