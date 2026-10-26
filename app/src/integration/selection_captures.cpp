#include "selection_captures.h"

#include <QClipboard>
#include <QGuiApplication>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

void markSource(CapturedText& text, const WindowTracker* tracker)
{
    if (tracker != nullptr && tracker->available()) {
        text.window = tracker->activeWindow();
        text.appName = tracker->activeApp();
    }
}

QString DataControlCapture::name() const
{
    return QObject::tr("Selection (Wayland data-control)");
}

std::optional<CapturedText> DataControlCapture::capture()
{
    beginCapture();
    CapturedText out;
    if (const auto selected = m_client->readText(DataControlClient::Which::Selection);
        selected && !selected->trimmed().isEmpty() && !isReplaced(*selected)) {
        out.text = *selected;
        out.origin = u"selection"_s;
    } else if (const auto copied = m_client->readText(DataControlClient::Which::Clipboard);
               copied && !copied->trimmed().isEmpty() && !isStaleClipboard(*copied)) {
        out.text = *copied;
        out.origin = u"clipboard"_s;
    } else {
        return std::nullopt;
    }
    markSource(out, m_tracker.get());
    return out;
}

QString X11SelectionCapture::name() const
{
    return QObject::tr("Selection (X11)");
}

bool X11SelectionCapture::available() const
{
    return QGuiApplication::platformName() == u"xcb"_s && QGuiApplication::clipboard()->supportsSelection();
}

std::optional<CapturedText> X11SelectionCapture::capture()
{
    beginCapture();
    const QClipboard* clipboard = QGuiApplication::clipboard();
    CapturedText out;
    if (const QString selected = clipboard->text(QClipboard::Selection);
        !selected.trimmed().isEmpty() && !isReplaced(selected)) {
        out.text = selected;
        out.origin = u"selection"_s;
    } else if (const QString copied = clipboard->text(QClipboard::Clipboard);
               !copied.trimmed().isEmpty() && !isStaleClipboard(copied)) {
        out.text = copied;
        out.origin = u"clipboard"_s;
    } else {
        return std::nullopt;
    }
    markSource(out, m_tracker.get());
    return out;
}

} // namespace paraphrase::app
