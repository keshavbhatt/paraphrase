#pragma once

#include <QLabel>
#include <QString>

namespace paraphrase::app {

/// One line of text that never widens its window: a long one is cut in the middle with "…",
/// and the whole text is in the tooltip. Selectable only while it shows in full, so a copy
/// never takes the "…" (the command to bind can be an AppImage's long path).
class ElidedLabel final : public QLabel
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ElidedLabel)

public:
    explicit ElidedLabel(const QString& text, QWidget* parent = nullptr);
    ~ElidedLabel() override = default;

    [[nodiscard]] QString fullText() const { return m_full; }
    [[nodiscard]] QSize minimumSizeHint() const override;
    [[nodiscard]] QSize sizeHint() const override;

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void elide();

    QString m_full;
};

} // namespace paraphrase::app
