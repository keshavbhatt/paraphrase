#pragma once

#include <QSplitter>
#include <QSplitterHandle>

#include <cstdint>

namespace paraphrase::app {

/// The handle of a GripSplitter: invisible, with a short rounded grip while the pointer is over
/// it or drags it (main-window-reply.html `.split`).
class GripHandle : public QSplitterHandle
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(GripHandle)

public:
    GripHandle(Qt::Orientation orientation, QSplitter* parent);
    ~GripHandle() override = default;

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool m_hovered = false;
    bool m_dragging = false;
};

/// A splitter whose handles show a grip on hover, for splits inside a pane.
class GripSplitter : public QSplitter
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(GripSplitter)

public:
    /// How a stack shares its height before the user drags the split.
    enum class Start : std::uint8_t
    {
        BottomSmallest, ///< the bottom box at its smallest, the top one takes the rest
        Even,           ///< half each
    };

    /// Between two boxes in a pane, the pane's 10px gaps on both sides of the grip.
    static constexpr int kPaneHandle = 20;

    explicit GripSplitter(Qt::Orientation orientation, QWidget* parent = nullptr);
    ~GripSplitter() override = default;

    /// `top` over `bottom` with a split between them (main-window-reply.html note 5): neither
    /// shrinks below its minimum height, extra height goes as `start` says, and the place the
    /// user drags it to is kept in QSettings under `settingsKey`.
    void stack(QWidget* top, QWidget* bottom, const QString& settingsKey, Start start);

protected:
    QSplitterHandle* createHandle() override;
};

} // namespace paraphrase::app
