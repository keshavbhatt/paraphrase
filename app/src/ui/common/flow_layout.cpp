#include "ui/common/flow_layout.h"

#include <QWidget>

#include <algorithm>

namespace paraphrase::app {

FlowLayout::FlowLayout(QWidget* parent, int spacing)
    : QLayout(parent)
    , m_spacing(spacing)
{
    setContentsMargins(0, 0, 0, 0);
}

FlowLayout::~FlowLayout()
{
    qDeleteAll(m_items);
}

void FlowLayout::addItem(QLayoutItem* item)
{
    m_items.append(item);
}

QLayoutItem* FlowLayout::takeAt(int index)
{
    return index >= 0 && index < m_items.size() ? m_items.takeAt(index) : nullptr;
}

int FlowLayout::heightForWidth(int width) const
{
    return arrange(QRect(0, 0, width, 0), false);
}

void FlowLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);
    (void)arrange(rect, true);
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (const QLayoutItem* item : m_items) {
        size = size.expandedTo(item->minimumSize());
    }
    return size;
}

int FlowLayout::arrange(const QRect& rect, bool apply) const
{
    int x = rect.x();
    int y = rect.y();
    int lineHeight = 0;
    for (QLayoutItem* item : m_items) {
        const QSize hint = item->sizeHint();
        if (x > rect.x() && x + hint.width() > rect.right() + 1) {
            x = rect.x();
            y += lineHeight + m_spacing;
            lineHeight = 0;
        }
        if (apply) {
            item->setGeometry(QRect(QPoint(x, y), hint));
        }
        x += hint.width() + m_spacing;
        lineHeight = std::max(lineHeight, hint.height());
    }
    return y + lineHeight - rect.y();
}

} // namespace paraphrase::app
