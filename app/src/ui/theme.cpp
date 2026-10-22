#include "theme.h"

#include <QFile>
#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>
#include <QStyleHints>
#include <QSvgRenderer>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

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
        .warning = QColor(0xB45309),
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
        .warning = QColor(0xF5B14C),
        .danger = QColor(0xFF8A80),
        .dark = true,
    };
}

Tokens Theme::current()
{
    const QString forced = qEnvironmentVariable("PARAPHRASE_THEME");
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
    const auto c = [](const QColor& color) { return color.name(); };
    // Sizes from DESIGN.md 1: 12 px labels, 14 px UI, 15 px text the user reads or changes;
    // 8 px controls, 10 px chips and result boxes, 12 px cards.
    return uR"(
QWidget#mainWindow { background: %1; color: %2; font-size: 14px; }
QLabel { color: %2; }
QLabel[role="muted"], QLabel[role="caption"] { color: %3; }
QLabel[role="caption"] { font-size: 11px; font-weight: 600; letter-spacing: 1px; }
QLabel[role="private"] { color: %4; }
QLabel[role="warning"] { color: %5; }
QLabel[role="danger"] { color: %6; }
QFrame#pane { background: %7; border: 1px solid %8; border-radius: 12px; }
QFrame#notice { background: %9; border: 1px solid %8; border-radius: 10px; }
QFrame#statusBar { background: %10; border-top: 1px solid %8; }
QPlainTextEdit, QTextBrowser, QLineEdit {
    background: %11; color: %2; border: 1px solid %8; border-radius: 10px; padding: 8px;
    font-size: 15px; selection-background-color: %12; }
QPlainTextEdit:focus, QLineEdit:focus { border: 2px solid %13; }
QTextBrowser { background: %14; }
QPushButton, QToolButton {
    background: transparent; color: %2; border: 1px solid %8; border-radius: 8px;
    padding: 6px 12px; min-height: 22px; }
QPushButton:hover, QToolButton:hover { background: %15; }
QPushButton:focus, QToolButton:focus { border: 2px solid %13; }
QPushButton:disabled { color: %3; }
QPushButton[kind="flat"] { border: none; }
QPushButton[kind="primary"] { background: %16; color: white; border: none; font-weight: 600; }
QPushButton[kind="primary"]:hover { background: %17; }
QPushButton[kind="primary"]:disabled { background: %15; color: %3; }
QPushButton[kind="action"]:checked, QToolButton[kind="action"]:checked {
    background: %9; border: 1.5px solid %13; color: %2; }
QToolButton::menu-indicator { image: none; width: 0; }
QPushButton[kind="choice"] { text-align: left; padding: 10px 12px; border-radius: 10px; font-size: 15px; }
QPushButton[kind="choice"]:checked { background: %9; border: 1.5px solid %13; }
QPushButton[kind="segment"] { border-radius: 0; padding: 4px 10px; }
QPushButton[kind="segment"]:checked { background: %9; font-weight: 600; }
QMenu { background: %18; color: %2; border: 1px solid %8; padding: 4px; }
QMenu::item { padding: 6px 18px; border-radius: 6px; }
QMenu::item:selected { background: %15; }
QSplitter::handle { background: transparent; }
QScrollArea, QScrollArea > QWidget > QWidget { background: transparent; border: none; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: %8; border-radius: 4px; min-height: 24px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
)"_s.arg(c(t.bg), c(t.text), c(t.muted), c(t.privacy), c(t.warning), c(t.danger), c(t.panel), c(t.border),
         c(t.accentSoft))
        .arg(c(t.bar), c(t.input), c(t.accentSoft), c(t.accent), c(t.sunk), c(t.hover), c(t.accentStrong),
             c(t.accentHover), c(t.elevated));
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
