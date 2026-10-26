#include "ui/common/elided_label.h"

#include <QFontMetrics>
#include <QResizeEvent>

namespace paraphrase::app {

ElidedLabel::ElidedLabel(const QString& text, QWidget* parent)
    : QLabel(text, parent)
    , m_full(text)
{
    setToolTip(text);
}

QSize ElidedLabel::minimumSizeHint() const
{
    return {fontMetrics().horizontalAdvance(QStringLiteral("…")) * 4, QLabel::minimumSizeHint().height()};
}

QSize ElidedLabel::sizeHint() const
{
    const QMargins m = contentsMargins();
    return {fontMetrics().horizontalAdvance(m_full) + m.left() + m.right() + (2 * margin()),
            QLabel::sizeHint().height()};
}

void ElidedLabel::resizeEvent(QResizeEvent* event)
{
    QLabel::resizeEvent(event);
    elide();
}

void ElidedLabel::elide()
{
    const QString shown =
        fontMetrics().elidedText(m_full, Qt::ElideMiddle, contentsRect().width() - (2 * margin()));
    if (shown != text()) {
        setText(shown);
    }
    setTextInteractionFlags(shown == m_full ? Qt::TextSelectableByMouse : Qt::NoTextInteraction);
}

} // namespace paraphrase::app
