#include "ui/common/flow_layout.h"

#include <QList>
#include <QWidget>

#include <algorithm>
#include <utility>

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
    // Items of one line share its centre: a 22 px pill beside a 34 px button lines up with
    // it instead of hanging from the top.
    QList<std::pair<QLayoutItem*, QPoint>> line;
    int lineHeight = 0;
    const auto place = [&line, &lineHeight, apply] {
        if (apply) {
            for (const auto& [item, at] : line) {
                const QSize hint = item->sizeHint();
                item->setGeometry(QRect(at + QPoint(0, (lineHeight - hint.height()) / 2), hint));
            }
        }
        line.clear();
    };
    int x = rect.x();
    int y = rect.y();
    for (QLayoutItem* item : m_items) {
        const QSize hint = item->sizeHint();
        if (x > rect.x() && x + hint.width() > rect.right() + 1) {
            place();
            x = rect.x();
            y += lineHeight + m_spacing;
            lineHeight = 0;
        }
        line.append({item, QPoint(x, y)});
        x += hint.width() + m_spacing;
        lineHeight = std::max(lineHeight, hint.height());
    }
    place();
    return y + lineHeight - rect.y();
}

} // namespace paraphrase::app
