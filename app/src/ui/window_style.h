#pragma once

#include "theme.h"

#include <QString>

class QLabel;
class QPushButton;

namespace paraphrase::app {

/// The style sheet of the setup and settings windows: the main window's (Theme::styleSheet)
/// plus their cards, steps, model cards, tables and navigation, from setup-model.html and
/// settings-models.html.
[[nodiscard]] QString windowStyleSheet(const Tokens& t);

/// Small builders shared by the two windows.
[[nodiscard]] QLabel* textLabel(const QString& text, const QString& role = {});
[[nodiscard]] QPushButton* plainButton(const QString& text, const QString& kind = {});
/// Re-applies the style sheet to a widget whose dynamic property changed.
void repolish(QWidget* w);
/// A rich text link in the theme's accent: QLabel links otherwise take the palette's blue.
[[nodiscard]] QString link(const QString& href, const QString& text, const Tokens& t);

} // namespace paraphrase::app
