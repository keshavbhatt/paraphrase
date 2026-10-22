#pragma once

#include <QWidget>

namespace paraphrase::app {

/// The setup window's first step: the global shortcut is not built yet, so it says what is
/// coming and that everything works in the main window meanwhile.
class ShortcutPage : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ShortcutPage)

public:
    explicit ShortcutPage(QWidget* parent = nullptr);
    ~ShortcutPage() override = default;
};

} // namespace paraphrase::app
