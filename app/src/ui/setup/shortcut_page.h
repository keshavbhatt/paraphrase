#pragma once

#include "ui/common/theme.h"
#include "ui/core/desktop_support.h"

#include <QWidget>

class QLabel;

namespace paraphrase::app {

class DesktopAccess;

/// The setup window's first step (setup-desktop.html): use Paraphrase over any app. The
/// shortcut and "Put the result back" (the desktop's permission to type), both optional:
/// everything also works in Paraphrase's own window (ADR-013).
class ShortcutPage : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ShortcutPage)

public:
    explicit ShortcutPage(QWidget* parent = nullptr);
    ~ShortcutPage() override = default;

    void setSupport(const DesktopSupport& support);
    void applyTheme(const Tokens& t);

private:
    DesktopAccess* m_access = nullptr;
    QLabel* m_lock = nullptr;
};

} // namespace paraphrase::app
