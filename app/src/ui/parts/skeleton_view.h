#pragma once

#include "ui/common/theme.h"

#include <QBrush>
#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>

class QPainter;

namespace paraphrase::app {

/// The shape of the answer to come, until its first word (main-window-busy.html): soft lines
/// with a slow shimmer in a box like the result's, or three cards for quick replies.
class SkeletonView : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(SkeletonView)

public:
    enum class Shape : std::uint8_t
    {
        Text,    ///< a paragraph: Proofread, Tone and the other text actions
        Draft,   ///< an email or message: Write and Full reply
        Replies, ///< three cards: quick replies
    };

    explicit SkeletonView(QWidget* parent = nullptr);
    ~SkeletonView() override = default;

    void setShape(Shape shape);
    /// The popup: no margins of its own, and a paragraph of `lines` lines, about as tall as the
    /// answer will be, so the popup hardly changes size when the answer comes.
    void setCompact(int lines);
    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] Shape shape() const { return m_shape; }
    void applyTheme(const Tokens& t);

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    [[nodiscard]] QBrush shimmer() const;
    void paintCards(QPainter& p, const QBrush& brush) const;

    Shape m_shape = Shape::Text;
    int m_lines = 0; ///< 0: the main window's fixed paragraph
    QElapsedTimer m_clock;
    QTimer m_tick;
    Tokens m_tokens;
};

} // namespace paraphrase::app
