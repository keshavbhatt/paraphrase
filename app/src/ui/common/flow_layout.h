#pragma once

#include <QLayout>
#include <QList>

namespace paraphrase::app {

/// Lays items out left to right and wraps them onto new lines, like text: the popup's
/// actions (popup-actions.html), whose number depends on the skills installed.
class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget* parent = nullptr, int spacing = 8);
    ~FlowLayout() override;
    FlowLayout(const FlowLayout&) = delete;
    FlowLayout& operator=(const FlowLayout&) = delete;
    FlowLayout(FlowLayout&&) = delete;
    FlowLayout& operator=(FlowLayout&&) = delete;

    void addItem(QLayoutItem* item) override;
    [[nodiscard]] int count() const override { return static_cast<int>(m_items.size()); }
    [[nodiscard]] QLayoutItem* itemAt(int index) const override { return m_items.value(index); }
    [[nodiscard]] QLayoutItem* takeAt(int index) override;
    [[nodiscard]] Qt::Orientations expandingDirections() const override { return {}; }
    [[nodiscard]] bool hasHeightForWidth() const override { return true; }
    [[nodiscard]] int heightForWidth(int width) const override;
    void setGeometry(const QRect& rect) override;
    [[nodiscard]] QSize sizeHint() const override { return minimumSize(); }
    [[nodiscard]] QSize minimumSize() const override;

private:
    /// Places the items in `rect` (or only measures, with `apply` false); returns the height.
    [[nodiscard]] int arrange(const QRect& rect, bool apply) const;

    QList<QLayoutItem*> m_items;
    int m_spacing;
};

} // namespace paraphrase::app
