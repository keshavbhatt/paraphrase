#pragma once

#include "ui/core/desktop_support.h"

#include <QWidget>

class QLabel;
class QVBoxLayout;

namespace paraphrase::app {

/// Settings, Shortcut (settings-general-shortcut.html, note 2): the shortcut and whether it
/// works here, how to bind it by hand, and what this desktop lets Paraphrase do.
class ShortcutSettingsPage : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ShortcutSettingsPage)

public:
    explicit ShortcutSettingsPage(QWidget* parent = nullptr);
    ~ShortcutSettingsPage() override = default;

    void setSupport(const DesktopSupport& support);

private:
    [[nodiscard]] static QWidget* shortcutCard(const DesktopSupport& support);
    [[nodiscard]] static QWidget* byHandCard(const DesktopSupport& support);
    [[nodiscard]] static QWidget* stepsCard(const DesktopSupport& support);

    QVBoxLayout* m_cards = nullptr;
};

} // namespace paraphrase::app
