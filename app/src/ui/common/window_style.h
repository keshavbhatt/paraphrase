#pragma once

#include "ui/common/theme.h"

#include <QString>

class QWidget;

namespace paraphrase::app {

/// The style sheet of the setup and settings windows: the main window's (Theme::styleSheet)
/// plus their cards, steps, model cards, tables and navigation, from setup-model.html and
/// settings-models.html.
[[nodiscard]] QString windowStyleSheet(const Tokens& t);

/// Re-applies the style sheet to a widget whose dynamic property changed.
void repolish(QWidget* w);
/// A rich text link in the theme's accent: QLabel links otherwise take the palette's blue.
[[nodiscard]] QString link(const QString& href, const QString& text, const Tokens& t);

} // namespace paraphrase::app
