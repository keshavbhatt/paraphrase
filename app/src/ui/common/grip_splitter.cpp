#include "ui/common/grip_splitter.h"

#include "ui/common/theme.h"

#include <QMouseEvent>
#include <QPainter>

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

void GripHandle::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    update();
    QSplitterHandle::enterEvent(event);
}

void GripHandle::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QSplitterHandle::leaveEvent(event);
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

QSplitterHandle* GripSplitter::createHandle()
{
    return new GripHandle(orientation(), this);
}

} // namespace paraphrase::app
