#include "ui/common/toggle_switch.h"

#include <QPainter>

namespace paraphrase::app {

ToggleSwitch::ToggleSwitch(QWidget* parent)
    : QAbstractButton(parent)
    , m_tokens(Theme::current())
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setFixedSize(sizeHint());
}

void ToggleSwitch::applyTheme(const Tokens& t)
{
    m_tokens = t;
    update();
}

void ToggleSwitch::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF track = QRectF(rect()).adjusted(1, 1, -1, -1);
    p.setPen(hasFocus() ? QPen(m_tokens.accent, 2) : Qt::NoPen);
    p.setBrush(isChecked() ? m_tokens.accentStrong : m_tokens.border);
    p.drawRoundedRect(track, track.height() / 2, track.height() / 2);
    const qreal knob = track.height() - 6;
    const qreal x = isChecked() ? track.right() - knob - 3 : track.left() + 3;
    p.setPen(Qt::NoPen);
    p.setBrush(m_tokens.panel);
    p.drawEllipse(QRectF(x, track.top() + 3, knob, knob));
}

} // namespace paraphrase::app
