#pragma once

#include "ui/common/theme.h"

#include <QWidget>

namespace paraphrase::util {
class LanguageDetector;
}

namespace paraphrase::app {

class PopupActionsPage;
class PopupResultPage;

/// The floating panel near the cursor (PLAN 3.2, popup-*.html): about 440 px wide, over any
/// app, with the actions page and the result page. Esc closes it from any state; Enter on a
/// result is Replace. It holds no logic: PopupController drives it.
class PopupWindow : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(PopupWindow)

public:
    static constexpr int kOffset = 12; ///< from the cursor

    explicit PopupWindow(const util::LanguageDetector& detector, QWidget* parent = nullptr);
    ~PopupWindow() override = default;

    [[nodiscard]] PopupActionsPage* actionsPage() const { return m_actions; }
    [[nodiscard]] PopupResultPage* resultPage() const { return m_result; }
    void showActions();
    void showResult();
    /// Shows the popup near `point` (the cursor), kept on its screen. Wayland may place it itself.
    void openNear(const QPoint& point);
    void applyTheme();

Q_SIGNALS:
    void closed();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    PopupActionsPage* m_actions = nullptr;
    PopupResultPage* m_result = nullptr;
    Tokens m_tokens;
    bool m_fitting = false;
};

} // namespace paraphrase::app
