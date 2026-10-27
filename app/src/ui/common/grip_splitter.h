#pragma once

#include <QSplitter>
#include <QSplitterHandle>

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
    explicit GripSplitter(Qt::Orientation orientation, QWidget* parent = nullptr);
    ~GripSplitter() override = default;

protected:
    QSplitterHandle* createHandle() override;
};

} // namespace paraphrase::app
