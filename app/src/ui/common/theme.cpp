#include "ui/common/theme.h"

#include <QFile>
#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>
#include <QSettings>
#include <QStyleHints>
#include <QSvgRenderer>

#include <cmath>
#include <utility>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

// Sizes and colours from the mocks (DOCS/mocks/mock.css and the main window pages):
// 11 px captions, 12 px small text, 14 px UI, 15 px text the user reads or changes;
// 8 px controls, 10 px actions, fields and big buttons, 12 px panes. @name@ is a token.
constexpr char16_t kSheet[] = uR"(
QWidget { font-size: 14px; }
QWidget#mainWindow { background: @bg@; color: @text@; font-size: 14px; }
QLabel { color: @text@; background: transparent; }
QLabel[role="muted"], QLabel[role="caption"], QLabel[role="small"] { color: @muted@; }
QLabel[role="small"] { font-size: 12px; }
QLabel[role="caption"] { font-size: 11px; font-weight: 700; letter-spacing: 1px; }
QLabel[role="private"] { color: @private@; font-size: 12px; }
QLabel[role="warning"] { color: @warning@; }
QLabel[role="danger"] { color: @danger@; }
QLabel[role="pill"] { background: @hover@; color: @muted@; border: 1px solid transparent; border-radius: 11px; padding: 2px 9px; font-size: 12px; }

QFrame#header { background: @bar@; border-bottom: 1px solid @border@; }
QFrame#notice { background: @warningBg@; border-bottom: 1px solid @border@; }
QFrame#notice QLabel { font-size: 13px; }
QFrame#statusBar { background: @bar@; border-top: 1px solid @border@; }
QFrame#statusBar QLabel { font-size: 12px; color: @muted@; }
QLabel#dot { background: @private@; border-radius: 4px; }

QFrame#pane { background: @panel@; border: 1px solid @border@; border-radius: 12px; }
QFrame#paneHead { border: none; border-bottom: 1px solid @border@; border-radius: 0; }
QFrame#paneFoot { border: none; border-top: 1px solid @border@; border-radius: 0; }

QPlainTextEdit, QTextEdit, QLineEdit {
    background: @input@; color: @text@; border: 1px solid @border@; border-radius: 10px;
    padding: 6px 8px; font-size: 15px; selection-background-color: @accentSoft@; selection-color: @text@; }
QLineEdit { border-radius: 8px; min-height: 26px; }
QPlainTextEdit:focus, QTextEdit[field="true"]:focus, QLineEdit:focus { border: 1px solid @accent@; }
QTextEdit#text, QTextBrowser#output { background: transparent; border: none; padding: 0; }

QPushButton, QToolButton {
    background: @panel@; color: @text@; border: 1px solid @border@; border-radius: 8px;
    padding: 0 14px; min-height: 34px; }
QPushButton:hover, QToolButton:hover { background: @hover@; }
QPushButton:focus { border: 1px solid @accent@; }
QPushButton:disabled { color: @muted@; }
QPushButton[kind="flat"] { background: transparent; border-color: transparent; }
QPushButton[kind="flat"]:hover { background: @hover@; }
QPushButton[kind="flat"]:checked { background: @accentSoft@; color: @accent@; }
QPushButton[kind="link"] { background: transparent; border-color: transparent; color: @link@; min-height: 26px; padding: 0 8px; }
QPushButton[kind="link"]:hover { background: @hover@; }
QWidget#installedModels QPushButton[kind="link"] { padding: 0 14px; }
QWidget#changes QPushButton[kind="link"] { font-size: 12px; min-height: 28px; padding: 0 8px; }
QPushButton[kind="icon"] { padding: 0; min-width: 32px; max-width: 32px; min-height: 32px; max-height: 32px; }
QPushButton[kind="icon"][flat="true"] { background: transparent; border-color: transparent; }
QPushButton[kind="icon"][flat="true"]:hover { background: @hover@; }
QPushButton[btnSize="big"] { min-height: 42px; border-radius: 10px; }
QPushButton[kind="primary"] { background: @accentStrong@; border-color: @accentStrong@; color: @accentText@; font-weight: 500; }
QPushButton[kind="primary"]:hover { background: @accentHover@; }
QPushButton[kind="primary"]:disabled { background: @primaryOff@; border-color: @primaryOff@; color: @primaryOffText@; }
QPushButton[kind="action"] { min-height: 42px; border-radius: 10px; font-weight: 500; padding: 0; }
QPushButton[kind="action"]:hover { border-color: @accent@; background: @panel@; }
QPushButton[kind="action"]:checked { background: @accentStrong@; border-color: @accentStrong@; }
QPushButton::menu-indicator { image: none; width: 0; }
QPushButton[kind="choice"] { min-height: 46px; border-radius: 10px; padding: 0; text-align: left; }
QPushButton[kind="choice"]:checked { background: @accentSoft@; border: 2px solid @accent@; }
QPushButton[kind="choice"] QLabel#label { font-size: 15px; font-weight: 400; }

QLabel#num { font-family: monospace; font-size: 11px; font-weight: 600; border-radius: 4px;
    padding: 1px 5px; background: @hover@; color: @muted@; }
QLabel#num[on="true"] { background: rgba(255, 255, 255, 51); color: @accentText@; }
QLabel#label { font-weight: 500; }
QLabel#label[on="true"] { color: @accentText@; }
QLabel#label[on="off"], QLabel#num[on="off"] { color: @primaryOffText@; }
QLabel#num[on="off"] { background: rgba(255, 255, 255, 51); }

QFrame#seg { background: @hover@; border-radius: 8px; }
QPushButton[kind="segment"] { background: transparent; border: none; border-radius: 6px; padding: 0 10px;
    min-height: 24px; font-size: 12px; color: @muted@; }
QPushButton[kind="segment"]:checked { background: @panel@; color: @text@; font-weight: 500; }

QLabel[chip="del"] { color: @del@; background: @delBg@; border-radius: 3px; padding: 0 2px;
    font-family: monospace; font-size: 12px; text-decoration: line-through; }
QLabel[chip="ins"] { color: @ins@; background: @insBg@; border-radius: 3px; padding: 0 2px;
    border-bottom: 2px solid @ins@; font-family: monospace; font-size: 12px; }
QLabel[chip="pill"] { color: @muted@; background: @hover@; border-radius: 9px; padding: 0 8px;
    min-height: 18px; max-height: 18px;
    font-family: monospace; font-size: 12px; }

QFrame#actionRow { border: none; border-bottom: 1px solid @border@; }
QFrame#actionRow[last="true"] { border-bottom: none; }
QLabel[role="cmd"] { font-family: monospace; font-size: 13px; background: @sunk@; border-radius: 8px; padding: 8px 10px; }
QFrame#popup { background: @panel@; border: 1px solid @border@; border-radius: 14px; }
QFrame#popupHead { border: none; border-bottom: 1px solid @border@; }
QFrame#chosenLanguage { background: @accentSoft@; border: none; border-radius: 12px; min-height: 24px; max-height: 24px; }
QLabel#chosenLanguageName { color: @accent@; font-size: 12px; font-weight: 600; }
QPushButton#clearLanguage { border: none; background: transparent; border-radius: 9px; padding: 0; min-height: 18px; }
QPushButton#clearLanguage:hover { background: @hover@; }
QFrame#popupFoot { border: none; border-top: 1px solid @border@; background: @bar@;
    border-bottom-left-radius: 14px; border-bottom-right-radius: 14px; }
QLabel#source { background: @sunk@; border-radius: 10px; padding: 11px 14px; font-size: 15px; }
QWidget#resultBox QTextBrowser#output { background: @panel@; border: 1px solid @border@; border-radius: 10px; padding: 12px 14px; }
QLabel#popupTitle { font-weight: 600; }
QLabel[role="kbd"] { border: 1px solid @border@; border-bottom-width: 2px; border-radius: 5px;
    padding: 0 6px; font-size: 12px; }
QPushButton[kind="tone"] { border-radius: 16px; min-height: 30px; padding: 0 12px; font-size: 13px; }
QPushButton[kind="tone"]:checked { background: @accentStrong@; border-color: @accentStrong@; color: @accentText@;
    font-weight: 500; }
QPushButton[kind="danger"] { background: transparent; border-color: transparent; color: @danger@; }
QPushButton[kind="danger"]:hover { background: @hover@; }
QPushButton[kind="dangerSolid"] { background: @danger@; border-color: @danger@; color: @accentText@; font-weight: 500; }
QPushButton[kind="dangerOutline"] { color: @danger@; }
QPushButton[kind="pill"] { background: @hover@; color: @muted@; border: none; border-radius: 11px; padding: 0 9px;
    min-height: 22px; max-height: 22px; font-size: 12px; }
QPushButton[kind="pill"]:hover { color: @text@; }
QFrame#confirm { border: 1px solid @danger@; border-radius: 10px; }
QPushButton[kind="filter"] { border-radius: 14px; min-height: 26px; max-height: 28px; padding: 0 11px; font-size: 12px; }
QPushButton[kind="filter"]:checked { background: @accentStrong@; border-color: @accentStrong@; color: @accentText@;
    font-weight: 500; }
QPlainTextEdit[role="box"] { padding: 10px 12px; }
QPlainTextEdit[role="box"][sunk="true"] { background: @sunk@; border-color: transparent; }
QListWidget#historyList { background: transparent; border: none; outline: 0; }
QFrame#search { background: @input@; border: 1px solid @border@; border-radius: 8px; }
QFrame#search QLineEdit { background: transparent; border: none; padding: 0; }
QLabel[role="h2"] { font-size: 15px; font-weight: 600; }
QLabel[role="day"] { color: @muted@; font-size: 11px; font-weight: 700; letter-spacing: 1px; }

QMenu { background: @elevated@; color: @text@; border: 1px solid @border@; border-radius: 8px; padding: 4px; }
QMenu::item { padding: 6px 18px; border-radius: 6px; }
QMenu::item:selected { background: @hover@; }
QMenu QListWidget#languageList { background: transparent; border: none; outline: 0; color: @text@; }
QMenu QListWidget#languageList::item { padding: 0 8px; border-radius: 6px; }
QMenu QListWidget#languageList::item:selected { background: @hover@; color: @text@; }
QSplitter::handle { background: transparent; }
QScrollArea, QScrollArea > QWidget > QWidget { background: transparent; border: none; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
QScrollBar::handle { background: @border@; border-radius: 3px; }
QScrollBar::handle:hover, QScrollBar::handle:pressed { background: @muted@; }
QScrollBar::handle:vertical { min-height: 24px; }
QScrollBar::handle:horizontal { min-width: 24px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; border: none; background: none; }
QScrollBar::add-page, QScrollBar::sub-page { background: none; }
QAbstractScrollArea::corner { background: transparent; }
)";

constexpr double kDisabledOpacity = 0.45;

QColor blend(const QColor& top, const QColor& under, double alpha)
{
    const auto mix = [alpha](int a, int b) {
        return static_cast<int>(std::lround((alpha * a) + ((1.0 - alpha) * b)));
    };
    return {mix(top.red(), under.red()), mix(top.green(), under.green()), mix(top.blue(), under.blue())};
}

} // namespace

Tokens Theme::light()
{
    return {
        .accent = QColor(0x2747C9),
        .accentStrong = QColor(0x2747C9),
        .accentHover = QColor(0x1B3399),
        .accentSoft = QColor(0xE3EAFF),
        .bg = QColor(0xF4F4F0),
        .bar = QColor(0xFFFFFF),
        .panel = QColor(0xFFFFFF),
        .elevated = QColor(0xFFFFFF),
        .sunk = QColor(0xF3F3EE),
        .hover = QColor(0xEEEEE8),
        .input = QColor(0xFFFFFF),
        .border = QColor(0xDCDCD5),
        .text = QColor(0x1C1D1F),
        .muted = QColor(0x5C5F66),
        .ins = QColor(0x1E3FA8),
        .insBg = QColor(0xE3EAFF),
        .del = QColor(0x9A3412),
        .delBg = QColor(0xFDEBDD),
        .privacy = QColor(0x3D6B4F),
        .success = QColor(0x2F6B45),
        .successBg = QColor(0xE4F0E7),
        .warning = QColor(0xB45309),
        .warningBg = QColor(0xFDF0DC),
        .danger = QColor(0xB42318),
        .dark = false,
    };
}

Tokens Theme::dark()
{
    return {
        .accent = QColor(0x9DB0FF),
        .accentStrong = QColor(0x3B5BDB),
        .accentHover = QColor(0x4C6CEB),
        .accentSoft = QColor(0x222C52),
        .bg = QColor(0x17181A),
        .bar = QColor(0x1E1F22),
        .panel = QColor(0x202124),
        .elevated = QColor(0x26272B),
        .sunk = QColor(0x2C2D31),
        .hover = QColor(0x313237),
        .input = QColor(0x151618),
        .border = QColor(0x393B41),
        .text = QColor(0xECEDEF),
        .muted = QColor(0xA3A6AD),
        .ins = QColor(0xB4C2FF),
        .insBg = QColor(0x26336A),
        .del = QColor(0xFDBA8C),
        .delBg = QColor(0x4A2716),
        .privacy = QColor(0x82C99E),
        .success = QColor(0x82C99E),
        .successBg = QColor(0x1F3A2A),
        .warning = QColor(0xF5B14C),
        .warningBg = QColor(0x3D2C12),
        .danger = QColor(0xFF8A80),
        .dark = true,
    };
}

Tokens Theme::current()
{
    QString forced = qEnvironmentVariable("PARAPHRASE_THEME");
    if (forced.isEmpty()) {
        forced = QSettings().value(u"general/appearance"_s).toString(); // Settings, General
    }
    if (forced == u"dark"_s) {
        return dark();
    }
    if (forced == u"light"_s) {
        return light();
    }
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark ? dark() : light();
}

QString Theme::styleSheet(const Tokens& t)
{
    QString css = QString::fromUtf16(kSheet);
    const std::pair<QStringView, QColor> colours[] = {
        {u"@bg@", t.bg},
        {u"@bar@", t.bar},
        {u"@panel@", t.panel},
        {u"@elevated@", t.elevated},
        {u"@hover@", t.hover},
        {u"@sunk@", t.sunk},
        {u"@input@", t.input},
        {u"@border@", t.border},
        {u"@text@", t.text},
        {u"@muted@", t.muted},
        {u"@accent@", t.accent},
        {u"@link@", t.accent},
        {u"@accentStrong@", t.accentStrong},
        {u"@accentHover@", t.accentHover},
        {u"@accentSoft@", t.accentSoft},
        {u"@accentText@", QColor(Qt::white)},
        {u"@private@", t.privacy},
        {u"@warning@", t.warning},
        {u"@warningBg@", t.warningBg},
        {u"@danger@", t.danger},
        {u"@ins@", t.ins},
        {u"@insBg@", t.insBg},
        {u"@del@", t.del},
        {u"@delBg@", t.delBg},
    };
    for (const auto& [name, colour] : colours) {
        css.replace(name.toString(), colour.name());
    }
    // A disabled primary button is the filled one at 45% (mock.css .btn:disabled), worked out
    // over the pane it sits on: a grey fill read as a different, plain button.
    const QColor off = blend(t.accentStrong, t.panel, kDisabledOpacity);
    css.replace(u"@primaryOffText@"_s, blend(QColor(Qt::white), off, kDisabledOpacity).name());
    css.replace(u"@primaryOff@"_s, off.name());
    return css;
}

QIcon Theme::icon(const QString& name, const QColor& color, int size, int padBefore, int padAfter)
{
    const QIcon plain = icon(name, color, size);
    if (plain.isNull()) {
        return {};
    }
    QIcon padded;
    for (const int scale : {1, 2}) {
        QPixmap pixmap((size + padBefore + padAfter) * scale, size * scale);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.drawPixmap(padBefore * scale, 0, plain.pixmap(size * scale, size * scale));
        painter.end();
        pixmap.setDevicePixelRatio(scale);
        padded.addPixmap(pixmap);
    }
    return padded;
}

QIcon Theme::icon(const QString& name, const QColor& color, int size)
{
    QFile file(u":/icons/"_s + name + u".svg"_s);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QByteArray svg = file.readAll();
    svg.replace("currentColor", color.name().toLatin1());
    QSvgRenderer renderer(svg);
    QIcon icon;
    for (const int scale : {1, 2}) {
        QPixmap pixmap(size * scale, size * scale);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        renderer.render(&painter);
        painter.end();
        pixmap.setDevicePixelRatio(scale);
        icon.addPixmap(pixmap);
    }
    return icon;
}

} // namespace paraphrase::app
