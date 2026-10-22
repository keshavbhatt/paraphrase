#pragma once

#include <QColor>
#include <QIcon>
#include <QString>

namespace paraphrase::app {

/// The design tokens (DOCS/DESIGN.md 1, the mocks' mock.css): one set per colour scheme.
struct Tokens
{
    QColor accent, accentStrong, accentHover, accentSoft;
    QColor bg, bar, panel, elevated, sunk, hover, input, border;
    QColor text, muted;
    QColor ins, insBg, del, delBg;
    QColor privacy, success, warning, danger;
    bool dark = false;
};

/// Light and dark, following the desktop's colour scheme, as DESIGN.md asks: both first class.
class Theme
{
public:
    [[nodiscard]] static Tokens light();
    [[nodiscard]] static Tokens dark();
    /// The scheme the desktop uses now (Qt's style hints), or the one forced by
    /// $PARAPHRASE_THEME ("light" or "dark", for screenshots and tests).
    [[nodiscard]] static Tokens current();

    /// The application's style sheet for these tokens.
    [[nodiscard]] static QString styleSheet(const Tokens& t);
    /// A Lucide icon from :/icons/<name>.svg, drawn in `color`.
    [[nodiscard]] static QIcon icon(const QString& name, const QColor& color, int size = 16);
};

} // namespace paraphrase::app
