#pragma once

#include <QPushButton>
#include <QString>

class QHBoxLayout;
class QLabel;

namespace paraphrase::app {

/// A button that shows a key chip ("Ctrl 1", "Ctrl Shift C", "2") beside its label, as the
/// mocks do (`.num`), with an optional icon before it and a menu arrow after it. QPushButton
/// paints plain text only, so the parts are child labels styled by the theme.
class ChipButton : public QPushButton
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ChipButton)

public:
    enum class ChipSide : std::uint8_t
    {
        Before, ///< actions and replies: "Ctrl 1 Proofread", "1 Yes, I'll come"
        After,  ///< the main button: "Copy result Ctrl Shift C"
    };

    explicit ChipButton(const QString& label, const QString& chip = {}, ChipSide side = ChipSide::Before,
                        QWidget* parent = nullptr);
    ~ChipButton() override = default;

    void setLabel(const QString& label);
    [[nodiscard]] QString label() const;
    void setChip(const QString& chip);
    void setLeadingIcon(const QIcon& icon);
    void setTrailingIcon(const QIcon& icon);
    /// Long labels (a reply) wrap instead of widening the window.
    void setWordWrap(bool wrap);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    bool event(QEvent* e) override;

private:
    /// The labels follow the button's state (checked, primary) through a property.
    void restyle();

    QHBoxLayout* m_layout = nullptr;
    QLabel* m_icon = nullptr;
    QLabel* m_chip = nullptr;
    QLabel* m_label = nullptr;
    QLabel* m_trailing = nullptr;
};

} // namespace paraphrase::app
