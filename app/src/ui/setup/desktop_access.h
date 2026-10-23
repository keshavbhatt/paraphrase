#pragma once

#include "ui/common/theme.h"
#include "ui/core/desktop_support.h"

#include <QFrame>

class QHBoxLayout;
class QLabel;
class QPushButton;
class QVBoxLayout;

namespace paraphrase::app {

/// The two things that make Paraphrase work over any app (setup-desktop.html): the shortcut,
/// and "Put the result back", the desktop's permission to type. In setup's first step and in
/// Settings, General and shortcut. Both are optional: everything also works in the window.
class DesktopAccess : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(DesktopAccess)

public:
    explicit DesktopAccess(QWidget* parent = nullptr);
    ~DesktopAccess() override = default;

    void setSupport(const DesktopSupport& support);
    /// Settings has its own shortcut cards: there only "Put the result back" shows.
    void setShowsShortcut(bool shows) const;
    void applyTheme(const Tokens& t);

private:
    struct Row
    {
        QFrame* card = nullptr;
        QLabel* badge = nullptr;
        QLabel* detail = nullptr;
        QVBoxLayout* side = nullptr;
    };
    [[nodiscard]] static Row buildRow(const QString& title, const QString& detail);
    void showShortcut();
    void showTyping() const;
    void allow();

    Row m_shortcut;
    Row m_typing;
    QPushButton* m_allow = nullptr;
    QLabel* m_pill = nullptr;
    QLabel* m_note = nullptr;
    DesktopSupport m_support;
    bool m_waiting = false;
    Tokens m_tokens;
};

} // namespace paraphrase::app
