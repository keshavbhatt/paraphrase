#include "ui/common/window_style.h"

#include <QStyle>
#include <QWidget>

#include <utility>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

// setup-model.html and settings-models.html on top of mock.css; @name@ is a token.
constexpr char16_t kWindowSheet[] = uR"(
QWidget#setupWindow, QWidget#settingsWindow { background: @bg@; color: @text@; font-size: 14px; }
QFrame#band { background: @bar@; border: none; border-bottom: 1px solid @border@; }
QFrame#footBand { background: @bar@; border: none; border-top: 1px solid @border@; }
QLabel[role="h1"] { font-size: 20px; font-weight: 600; }
QLabel[role="h2"] { font-size: 15px; font-weight: 600; }
QLabel[role="title"] { font-size: 16px; font-weight: 600; }
QLabel[role="mono"] { font-family: monospace; font-size: 12px; color: @text@; }
QLabel[role="success"] { color: @success@; font-size: 12px; }
QLabel[role="pill-accent"] { background: @accentSoft@; color: @accent@; border: 1px solid transparent; border-radius: 11px; padding: 2px 9px; font-size: 12px; }
QLabel[role="pill-ok"] { background: @successBg@; color: @success@; border: 1px solid transparent; border-radius: 11px; padding: 2px 9px; font-size: 12px; }
QLabel[role="pill-warn"] { background: @warningBg@; color: @warning@; border: 1px solid transparent; border-radius: 11px; padding: 2px 9px; font-size: 12px; }
QRadioButton::indicator { width: 14px; height: 14px; border-radius: 8px; border: 1px solid @border@; background: @input@; }
QRadioButton::indicator:checked { width: 6px; height: 6px; border: 5px solid @accentStrong@; background: @panel@; }
QFrame[role="opt"] { border: 1px solid @border@; border-radius: 10px; background: transparent; padding: 1px; }
QFrame[role="opt"][on="true"] { border: 2px solid @accent@; background: @accentSoft@; padding: 0; }
QFrame[role="grant"] { background: @panel@; border: 1px solid @border@; border-radius: 12px; }
QLabel[role="grantBadge"] { background: @accentSoft@; border-radius: 10px; }
QLabel[role="th"] { color: @muted@; font-size: 11px; font-weight: 700; letter-spacing: 1px; }

QLabel[step="todo"] { color: @muted@; font-size: 13px; }
QLabel[step="on"] { color: @text@; font-size: 13px; font-weight: 600; }
QLabel[step="done"] { color: @success@; font-size: 13px; }
QLabel[badge="todo"] { color: @muted@; border: 1px solid @border@; border-radius: 9px; font-family: monospace; font-size: 11px; font-weight: 600; }
QLabel[badge="done"] { border: none; background: transparent; }
QLabel[badge="on"] { color: @accentText@; background: @accentStrong@; border: 1px solid @accentStrong@; border-radius: 9px; font-family: monospace; font-size: 11px; font-weight: 600; }

QFrame#card { background: @panel@; border: 1px solid @border@; border-radius: 12px; }
QFrame#modelCard { background: @panel@; border: 1px solid @border@; border-radius: 12px; padding: 1px; }
QFrame#modelCard[on="true"] { border: 2px solid @accent@; padding: 0; }
QFrame#modelCard[enabled="false"] { background: @sunk@; }
QFrame#lang { border: 1px solid @border@; border-radius: 10px; }
QLabel[role="strong"] { font-weight: 600; }
QFrame#rule { border: none; background: @border@; max-height: 1px; min-height: 1px; }

QProgressBar { background: @border@; border: none; border-radius: 3px; max-height: 6px; min-height: 6px; }
QProgressBar::chunk { background: @accentStrong@; border-radius: 3px; }

QCheckBox { spacing: 8px; }
QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid @border@; border-radius: 4px; background: @input@; }
QCheckBox::indicator:hover { border-color: @accent@; }
QCheckBox::indicator:checked { background: @accentStrong@; border-color: @accentStrong@; image: url(:/icons/check-white.svg); }
QCheckBox::indicator:disabled { background: @sunk@; }
QFrame#modelCard QCheckBox { spacing: 0; }

QComboBox { background: @input@; color: @text@; border: 1px solid @border@; border-radius: 8px; padding: 0 10px; min-height: 36px; }
QComboBox:focus { border: 1px solid @accent@; }
QLabel[role="pill"] { border: 1px solid transparent; border-radius: 11px; padding: 2px 9px; }
QComboBox::drop-down { border: none; background: transparent; width: 28px; }
QComboBox::down-arrow { image: url(:/icons/down-@scheme@.svg); width: 14px; height: 14px; }
QComboBox QAbstractItemView { background: @elevated@; color: @text@; selection-background-color: @hover@; selection-color: @text@;
    border: 1px solid @border@; border-radius: 8px; padding: 4px; outline: 0; }
QComboBox QAbstractItemView::item { min-height: 30px; padding: 0 10px; border-radius: 6px; border: none; }
QComboBox QAbstractItemView::item:selected, QComboBox QAbstractItemView::item:hover { background: @hover@; color: @text@; }
QComboBoxPrivateContainer { background: transparent; border: none; padding: 0; margin: 0; }

QPushButton[kind="nav"] { text-align: left; background: transparent; border: none; border-radius: 8px; padding: 0 12px; min-height: 38px; }
QPushButton[kind="nav"]:hover { background: @hover@; }
QPushButton[kind="nav"]:checked { background: @accentSoft@; color: @accent@; font-weight: 500; }
)";

} // namespace

QString windowStyleSheet(const Tokens& t)
{
    QString css = QString::fromUtf16(kWindowSheet);
    const std::pair<QStringView, QColor> colours[] = {
        {u"@bg@", t.bg},
        {u"@bar@", t.bar},
        {u"@panel@", t.panel},
        {u"@elevated@", t.elevated},
        {u"@sunk@", t.sunk},
        {u"@hover@", t.hover},
        {u"@input@", t.input},
        {u"@border@", t.border},
        {u"@text@", t.text},
        {u"@muted@", t.muted},
        {u"@accent@", t.accent},
        {u"@accentStrong@", t.accentStrong},
        {u"@accentSoft@", t.accentSoft},
        {u"@accentText@", QColor(Qt::white)},
        {u"@success@", t.success},
        {u"@successBg@", t.successBg},
        {u"@warning@", t.warning},
        {u"@warningBg@", t.warningBg},
        {u"@danger@", t.danger},
    };
    for (const auto& [name, colour] : colours) {
        css.replace(name.toString(), colour.name());
    }
    css.replace(u"@scheme@"_s, t.dark ? u"dark"_s : u"light"_s);
    return Theme::styleSheet(t) + css;
}

QString link(const QString& href, const QString& text, const Tokens& t)
{
    return u"<a href=\"%1\" style=\"color: %2; text-decoration: none\">%3</a>"_s.arg(href.toHtmlEscaped(),
                                                                                     t.accent.name(), text);
}

void repolish(QWidget* w)
{
    w->style()->unpolish(w);
    w->style()->polish(w);
}

} // namespace paraphrase::app
