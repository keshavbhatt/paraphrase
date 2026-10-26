#pragma once

#include "ui/common/theme.h"

#include <QIcon>
#include <QProxyStyle>
#include <QString>

namespace paraphrase::app {

/// Menu item icons (menus.html): every menu draws its icons this size, so a language's script
/// stays readable.
inline constexpr int kMenuIconSize = 20;

/// The Lucide icon for a tone or format preset ("professional", "bullets"), or "" if none.
[[nodiscard]] QString presetIcon(const QString& preset);
/// The Lucide icon for an action, by its skill id: History's badges and the run button.
[[nodiscard]] QString actionIcon(const QString& skillId);
/// A Lucide icon in a menu's icon box, muted.
[[nodiscard]] QIcon menuIcon(const QString& name, const Tokens& t);
/// A language's badge: a letter of its own script on a soft square ("अ" for Hindi), never a
/// flag, since a language is not a country.
[[nodiscard]] QIcon scriptBadge(const QString& code, const Tokens& t);
/// The language's name in its own writing ("हिन्दी"), or "" when it is the same as in English.
[[nodiscard]] QString nativeLanguageName(const QString& code);
/// One of the user's own actions: the first letter of its name on the accent's soft colour.
[[nodiscard]] QIcon letterBadge(const QString& name, const Tokens& t);

/// The application style: menus draw their icons at kMenuIconSize and have the mocks' round
/// corners (a menu window is square unless its background is see-through).
class MenuIconStyle : public QProxyStyle
{
    Q_OBJECT

public:
    using QProxyStyle::QProxyStyle;

    [[nodiscard]] int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                                  const QWidget* widget = nullptr) const override;
    void polish(QWidget* widget) override;
    using QProxyStyle::polish;
};

} // namespace paraphrase::app
