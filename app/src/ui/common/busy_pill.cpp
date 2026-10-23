#include "ui/common/busy_pill.h"

#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kHeight = 26;
constexpr int kMinWidth = 218; // main-window-busy.html: .busy { min-width }
constexpr int kPadLeft = 9;
constexpr int kPadRight = 11;
constexpr int kDot = 8;
constexpr int kGap = 8;
constexpr int kFontPx = 12;
constexpr int kFrameMs = 40;
constexpr int kPulseMs = 1400; // @keyframes ring

} // namespace

BusyPill::BusyPill(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(u"busyPill"_s);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAccessibleName(tr("Working"));
    m_tick.setInterval(kFrameMs);
    connect(&m_tick, &QTimer::timeout, this, qOverload<>(&QWidget::update));
    hide();
}

void BusyPill::setSteps(const QStringList& steps)
{
    m_steps = steps;
    updateGeometry();
}

void BusyPill::start(const QString& step)
{
    m_clock.start();
    setStep(step);
    show();
    m_tick.start();
}

void BusyPill::setStep(const QString& step)
{
    m_step = step;
    setAccessibleDescription(step);
    update();
}

void BusyPill::stop()
{
    m_tick.stop();
    hide();
}

void BusyPill::applyTheme(const Tokens& t)
{
    m_tokens = t;
    update();
}

QFont BusyPill::stepFont() const
{
    QFont f = font();
    f.setPixelSize(kFontPx);
    f.setWeight(QFont::DemiBold);
    return f;
}

QString BusyPill::seconds() const
{
    const qint64 s = m_clock.isValid() ? m_clock.elapsed() / 1000 : 0;
    return tr("%1 s").arg(s);
}

QSize BusyPill::sizeHint() const
{
    const QFontMetrics bold(stepFont());
    QFont plain = stepFont();
    plain.setWeight(QFont::Normal);
    int widest = 0;
    for (const QString& s : m_steps) {
        widest = std::max(widest, bold.horizontalAdvance(s));
    }
    const int width = kPadLeft + kDot + kGap + widest + kGap +
                      QFontMetrics(plain).horizontalAdvance(tr("%1 s").arg(99)) + kPadRight;
    return {std::max(kMinWidth, width), kHeight};
}

void BusyPill::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF box(0, (height() - kHeight) / 2.0, width(), kHeight);
    QPainterPath shape;
    shape.addRoundedRect(box, kHeight / 2.0, kHeight / 2.0);
    p.fillPath(shape, m_tokens.accentSoft);

    // The dot, and a ring that grows from it and fades (mock: .pulse::after).
    const QPointF centre(box.left() + kPadLeft + (kDot / 2.0), box.center().y());
    p.setPen(Qt::NoPen);
    p.setBrush(m_tokens.accent);
    p.drawEllipse(centre, kDot / 2.0, kDot / 2.0);
    if (m_clock.isValid()) {
        const double t = static_cast<double>(m_clock.elapsed() % kPulseMs) / kPulseMs;
        const double eased = 1.0 - std::pow(1.0 - t, 2.0); // ease-out
        const double radius = ((kDot / 2.0) + 4.0) * (0.4 + (1.1 * eased));
        QColor ring = m_tokens.accent;
        ring.setAlphaF(static_cast<float>(0.9 * (1.0 - t)));
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(ring, 2));
        p.drawEllipse(centre, radius, radius);
    }

    const int textLeft = kPadLeft + kDot + kGap;
    const QRect textBox(textLeft, static_cast<int>(box.top()), width() - textLeft - kPadRight, kHeight);
    p.setPen(m_tokens.accent);
    p.setFont(stepFont());
    p.drawText(textBox, Qt::AlignLeft | Qt::AlignVCenter, m_step);
    QFont plain = stepFont();
    plain.setWeight(QFont::Normal);
    p.setFont(plain);
    p.drawText(textBox, Qt::AlignRight | Qt::AlignVCenter, seconds());
}

} // namespace paraphrase::app
