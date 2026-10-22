#include "window_style.h"

#include <QLabel>
#include <QPushButton>
#include <QStyle>

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
QLabel[role="pill-accent"] { background: @accentSoft@; color: @accent@; border-radius: 11px; padding: 2px 9px; font-size: 12px; }
QLabel[role="pill-ok"] { background: @successBg@; color: @success@; border-radius: 11px; padding: 2px 9px; font-size: 12px; }
QLabel[role="pill-warn"] { background: @warningBg@; color: @warning@; border-radius: 11px; padding: 2px 9px; font-size: 12px; }
QLabel[role="th"] { color: @muted@; font-size: 11px; font-weight: 700; letter-spacing: 1px; }

QLabel[step="todo"] { color: @muted@; font-size: 13px; }
QLabel[step="on"] { color: @text@; font-size: 13px; font-weight: 600; }
QLabel[step="done"] { color: @success@; font-size: 13px; }
QLabel[badge="todo"] { color: @muted@; border: 1px solid @border@; border-radius: 9px; font-family: monospace; font-size: 11px; font-weight: 600; }
QLabel[badge="on"] { color: @accentText@; background: @accentStrong@; border: 1px solid @accentStrong@; border-radius: 9px; font-family: monospace; font-size: 11px; font-weight: 600; }

QFrame#card { background: @panel@; border: 1px solid @border@; border-radius: 12px; }
QFrame#modelCard { background: @panel@; border: 1px solid @border@; border-radius: 12px; }
QFrame#modelCard[on="true"] { border: 2px solid @accent@; }
QFrame#modelCard[enabled="false"] { background: @sunk@; }
QFrame#lang { border: 1px solid @border@; border-radius: 10px; }
QFrame#rule { border: none; background: @border@; max-height: 1px; min-height: 1px; }

QProgressBar { background: @border@; border: none; border-radius: 3px; max-height: 6px; min-height: 6px; }
QProgressBar::chunk { background: @accentStrong@; border-radius: 3px; }

QCheckBox { spacing: 8px; }
QCheckBox::indicator { width: 18px; height: 18px; }

QComboBox { background: @input@; color: @text@; border: 1px solid @border@; border-radius: 8px; padding: 0 10px; min-height: 36px; }
QComboBox:focus { border: 1px solid @accent@; }
QComboBox::drop-down { border: none; background: transparent; width: 28px; }
QComboBox QAbstractItemView { background: @elevated@; color: @text@; selection-background-color: @hover@; selection-color: @text@; }

QPushButton[kind="nav"] { text-align: left; background: transparent; border: none; border-radius: 8px; padding: 0 12px; min-height: 38px; }
QPushButton[kind="nav"]:hover { background: @hover@; }
QPushButton[kind="nav"]:checked { background: @accentSoft@; color: @accent@; font-weight: 500; }
QPushButton[kind="danger"] { background: transparent; border-color: transparent; color: @danger@; }
QPushButton[kind="danger"]:hover { background: @hover@; }
QPushButton[kind="dangerSolid"] { background: @danger@; border-color: @danger@; color: @accentText@; font-weight: 500; }
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
    return Theme::styleSheet(t) + css;
}

QLabel* textLabel(const QString& text, const QString& role)
{
    auto* l = new QLabel(text);
    if (!role.isEmpty()) {
        l->setProperty("role", role);
    }
    return l;
}

QPushButton* plainButton(const QString& text, const QString& kind)
{
    auto* b = new QPushButton(text);
    if (!kind.isEmpty()) {
        b->setProperty("kind", kind);
    }
    b->setCursor(Qt::PointingHandCursor);
    return b;
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
