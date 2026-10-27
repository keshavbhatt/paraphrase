#include "ui/common/grip_splitter.h"

#include "ui/common/theme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QSettings>

namespace paraphrase::app {
namespace {

constexpr int kGripLength = 36; ///< .split::after { width: 36px; height: 3px }
constexpr int kGripThickness = 3;

} // namespace

GripHandle::GripHandle(Qt::Orientation orientation, QSplitter* parent)
    : QSplitterHandle(orientation, parent)
{
    setAttribute(Qt::WA_Hover);
}

void GripHandle::paintEvent(QPaintEvent* /*event*/)
{
    if (!m_hovered && !m_dragging) {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Theme::current().border);
    const bool across = orientation() == Qt::Vertical; // a vertical splitter's handle is a row
    QRectF grip(0, 0, across ? kGripLength : kGripThickness, across ? kGripThickness : kGripLength);
    grip.moveCenter(QRectF(rect()).center());
    painter.drawRoundedRect(grip, kGripThickness / 2.0, kGripThickness / 2.0);
}

bool GripHandle::event(QEvent* event)
{
    // Breeze shows a proxy over a hovered splitter handle to widen its grip, which takes the
    // pointer, so the handle gets no Enter or Leave: only Breeze's HoverEnter and, when the
    // proxy goes, a HoverLeave (breezesplitterproxy.cpp). Either pair shows the grip.
    switch (event->type()) {
    case QEvent::Enter:
    case QEvent::HoverEnter:
        setHovered(true);
        break;
    case QEvent::Leave:
    case QEvent::HoverLeave:
        setHovered(false);
        break;
    default:
        break;
    }
    return QSplitterHandle::event(event);
}

void GripHandle::setHovered(bool hovered)
{
    if (m_hovered != hovered) {
        m_hovered = hovered;
        update();
    }
}

void GripHandle::mousePressEvent(QMouseEvent* event)
{
    m_dragging = event->button() == Qt::LeftButton;
    update();
    QSplitterHandle::mousePressEvent(event);
}

void GripHandle::mouseReleaseEvent(QMouseEvent* event)
{
    m_dragging = false;
    update();
    QSplitterHandle::mouseReleaseEvent(event);
}

GripSplitter::GripSplitter(Qt::Orientation orientation, QWidget* parent)
    : QSplitter(orientation, parent)
{}

void GripSplitter::stack(QWidget* top, QWidget* bottom, const QString& settingsKey, Start start)
{
    setChildrenCollapsible(false);
    setHandleWidth(kPaneHandle);
    addWidget(top);
    addWidget(bottom);
    const bool even = start == Start::Even;
    setStretchFactor(0, 1);
    setStretchFactor(1, even ? 1 : 0);
    if (!restoreState(QSettings().value(settingsKey).toByteArray())) {
        // Sizes are scaled to the room there is; 0 is held at the bottom box's minimum.
        setSizes(even ? QList<int>{1, 1} : QList<int>{QWIDGETSIZE_MAX, 0});
    }
    connect(this, &QSplitter::splitterMoved, this,
            [this, settingsKey] { QSettings().setValue(settingsKey, saveState()); });
}

QSplitterHandle* GripSplitter::createHandle()
{
    return new GripHandle(orientation(), this);
}

} // namespace paraphrase::app
