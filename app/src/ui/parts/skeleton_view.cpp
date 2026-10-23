#include "ui/parts/skeleton_view.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <array>
#include <span>
#include <vector>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kMargin = 14;    // the result views' own margins
constexpr int kTextTop = 12;   // TextResultView's top margin
constexpr int kInset = 14;     // inside the box, as the text's padding
constexpr int kBar = 12;       // .sk i { height }
constexpr int kRowGap = 12;    // .sk { gap }
constexpr int kCardRowGap = 9; // .skcard .sk { gap }
constexpr int kGapRow = 6;     // .sk .gap: a blank line between paragraphs
constexpr int kCardGap = 10;   // the replies' row spacing
constexpr int kFrameMs = 40;
constexpr int kShimmerMs = 1600; // @keyframes shimmer

// Line widths in percent, 0 for a blank line (main-window-busy.html).
constexpr std::array kDraft{42, 0, 22, 0, 96, 89, 64, 0, 80, 35};
constexpr std::array kText{96, 89, 92, 64, 0, 80, 45};
constexpr std::array kCard1{88, 40};
constexpr std::array kCard2{72};
constexpr std::array kCard3{94, 55};

} // namespace

SkeletonView::SkeletonView(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(u"skeleton"_s);
    setAccessibleName(tr("The answer is being written"));
    m_tick.setInterval(kFrameMs);
    connect(&m_tick, &QTimer::timeout, this, qOverload<>(&QWidget::update));
}

void SkeletonView::setShape(Shape shape)
{
    m_shape = shape;
    update();
}

void SkeletonView::setCompact(int lines)
{
    m_lines = std::max(1, lines);
    updateGeometry();
    update();
}

QSize SkeletonView::sizeHint() const
{
    if (m_lines == 0) {
        return QWidget::sizeHint();
    }
    if (m_shape == Shape::Replies) {
        // Three cards of two, one and two lines (paintCards).
        return {QWidget::sizeHint().width(), (6 * kInset) + (5 * kBar) + (2 * kCardRowGap) + (2 * kCardGap)};
    }
    return {QWidget::sizeHint().width(), (m_lines * (kBar + kRowGap)) - kRowGap + (2 * kInset) + 4};
}

void SkeletonView::applyTheme(const Tokens& t)
{
    m_tokens = t;
    update();
}

void SkeletonView::showEvent(QShowEvent* event)
{
    m_clock.start();
    m_tick.start();
    QWidget::showEvent(event);
}

void SkeletonView::hideEvent(QHideEvent* event)
{
    m_tick.stop();
    QWidget::hideEvent(event);
}

namespace {

/// Rounded bars, `percent` of the area's width each; 0 is a blank line between paragraphs.
void paintBars(QPainter& p, const QRectF& area, std::span<const int> widths, int gap, const QBrush& brush)
{
    double y = area.top();
    for (const int percent : widths) {
        if (percent == 0) {
            y += kGapRow + gap;
            continue;
        }
        QPainterPath bar;
        bar.addRoundedRect(QRectF(area.left(), y, area.width() * percent / 100.0, kBar), kBar / 2.0,
                           kBar / 2.0);
        p.fillPath(bar, brush);
        y += kBar + gap;
    }
}

void paintOutline(QPainter& p, const QRectF& r, const QColor& border, const QBrush& fill)
{
    QPainterPath path;
    path.addRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), 10, 10);
    p.fillPath(path, fill);
    p.setPen(QPen(border, 1));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
}

} // namespace

QBrush SkeletonView::shimmer() const
{
    // One light band moves across every line at once (mock: background-position).
    const double t = m_clock.isValid() ? static_cast<double>(m_clock.elapsed() % kShimmerMs) / kShimmerMs : 0;
    const double w = width();
    const double centre = (-0.3 * w) + (t * 1.6 * w);
    QLinearGradient band(centre - (0.25 * w), 0, centre + (0.25 * w), 0);
    band.setColorAt(0, m_tokens.border);
    band.setColorAt(0.5, m_tokens.hover);
    band.setColorAt(1, m_tokens.border);
    return band;
}

void SkeletonView::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QBrush brush = shimmer();
    if (m_shape == Shape::Replies) {
        paintCards(p, brush);
        return;
    }
    if (m_lines > 0) {
        // The popup: a paragraph as long as the text, in the body's own margins.
        std::vector<int> widths;
        widths.reserve(static_cast<std::size_t>(m_lines));
        for (int i = 0; i < m_lines; ++i) {
            widths.push_back(i + 1 == m_lines && m_lines > 1 ? 58
                                                             : kText.at(static_cast<std::size_t>(i) % 4));
        }
        // In the result box's own outline, as the answer will be (popup: #resultBox).
        paintOutline(p, QRectF(0, 0, width(), height()), m_tokens.border, m_tokens.input);
        paintBars(p, QRectF(kInset, kInset + 2, width() - (2 * kInset), height() - (2 * kInset)), widths,
                  kRowGap, brush);
        return;
    }
    const int top = m_shape == Shape::Text ? kTextTop : kMargin;
    const QRectF r(kMargin, top, width() - (2 * kMargin), height() - top - kMargin);
    if (m_shape == Shape::Draft) {
        // The draft is an edit box; a text result has none (QTextBrowser#output).
        paintOutline(p, r, m_tokens.border, m_tokens.input);
        paintBars(p, r.adjusted(kInset, kInset + 4, -kInset, -kInset), kDraft, kRowGap, brush);
    } else {
        paintBars(p, r.adjusted(0, 4, 0, 0), kText, kRowGap, brush);
    }
}

void SkeletonView::paintCards(QPainter& p, const QBrush& brush) const
{
    // Three cards with their numbers (mock: .skcard).
    const std::array<std::span<const int>, 3> cards{kCard1, kCard2, kCard3};
    const int margin = m_lines > 0 ? 0 : kMargin;
    double y = margin;
    QFont number = font();
    number.setFamily(u"monospace"_s);
    number.setPixelSize(11);
    number.setWeight(QFont::DemiBold);
    for (std::size_t i = 0; i < cards.size(); ++i) {
        const auto lines = static_cast<double>(cards.at(i).size());
        const double h = (2 * kInset) + (lines * kBar) + ((lines - 1) * kCardRowGap);
        const QRectF card(margin, y, width() - (2 * margin), h);
        paintOutline(p, card, m_tokens.border, Qt::NoBrush);
        const QRectF chip(card.left() + kInset, card.top() + kInset - 1, 18, 18);
        QPainterPath chipPath;
        chipPath.addRoundedRect(chip, 4, 4);
        p.fillPath(chipPath, m_tokens.hover);
        p.setPen(m_tokens.muted);
        p.setFont(number);
        p.drawText(chip, Qt::AlignCenter, QString::number(i + 1));
        const double left = chip.right() + 12;
        paintBars(p, QRectF(left, card.top() + kInset, card.right() - kInset - left, h), cards.at(i),
                  kCardRowGap, brush);
        y += h + kCardGap;
    }
}

} // namespace paraphrase::app
