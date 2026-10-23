#pragma once

#include "ui/common/theme.h"

#include <QIcon>
#include <QMenu>
#include <QProxyStyle>
#include <QString>

namespace paraphrase::app {

/// Menu item icons (menus.html): every menu draws its icons this size, so a language's script
/// stays readable.
inline constexpr int kMenuIconSize = 20;

/// The Lucide icon for a tone or format preset ("professional", "bullets"), or "" if none.
[[nodiscard]] QString presetIcon(const QString& preset);
/// A Lucide icon in a menu's icon box, muted.
[[nodiscard]] QIcon menuIcon(const QString& name, const Tokens& t);
/// A language's badge: a letter of its own script on a soft square ("अ" for Hindi), never a
/// flag, since a language is not a country.
[[nodiscard]] QIcon scriptBadge(const QString& code, const Tokens& t);
/// The language's name in its own writing ("हिन्दी"), or "" when it is the same as in English.
[[nodiscard]] QString nativeLanguageName(const QString& code);
/// One of the user's own actions: the first letter of its name on the accent's soft colour.
[[nodiscard]] QIcon letterBadge(const QString& name, const Tokens& t);

/// The application style with menus drawing their icons at kMenuIconSize.
class MenuIconStyle : public QProxyStyle
{
    Q_OBJECT

public:
    using QProxyStyle::QProxyStyle;

    [[nodiscard]] int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                                  const QWidget* widget = nullptr) const override;
};

/// A menu that shows each language's own name, muted, at the right edge (menus.html). The
/// style sheet draws menu items itself, so the names are drawn over the finished menu.
class NativeNamesMenu : public QMenu
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(NativeNamesMenu)

public:
    explicit NativeNamesMenu(QWidget* parent = nullptr);
    ~NativeNamesMenu() override = default;

    void setNativeName(QAction* action, const QString& native);
    /// A heading between items ("BEST EFFORT"), as menus.html shows it.
    void addCaption(const QString& text);
    void setMuted(const QColor& muted) { m_muted = muted; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor m_muted;
};

} // namespace paraphrase::app
