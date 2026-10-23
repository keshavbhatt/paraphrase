#include "ui/common/menu_icons.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QCoreApplication>
#include <QHash>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyledItemDelegate>

#include <algorithm>
#include <functional>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kGlyph = 16;          // the Lucide icon inside the menu's icon box
constexpr int kNativeRightPad = 14; // QMenu::item's right padding in theme.cpp
constexpr int kNativeGap = 32;      // between a language's name and its own name
constexpr int kNativePx = 12;

QIcon drawn(const std::function<void(QPainter&)>& paint)
{
    QIcon icon;
    for (const int scale : {1, 2}) {
        QPixmap pixmap(kMenuIconSize * scale, kMenuIconSize * scale);
        pixmap.fill(Qt::transparent);
        pixmap.setDevicePixelRatio(scale);
        QPainter p(&pixmap);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);
        paint(p);
        p.end();
        icon.addPixmap(pixmap);
    }
    return icon;
}

QIcon badge(const QString& text, const QRectF& shape, qreal radius, const QColor& fill, const QColor& ink,
            int pixelSize, QFont::Weight weight)
{
    return drawn([&](QPainter& p) {
        QPainterPath path;
        path.addRoundedRect(shape, radius, radius);
        p.fillPath(path, fill);
        QFont f = QFont();
        f.setPixelSize(pixelSize);
        f.setWeight(weight);
        p.setFont(f);
        p.setPen(ink);
        p.drawText(shape, Qt::AlignCenter, text);
    });
}

} // namespace

QString presetIcon(const QString& preset)
{
    static const QHash<QString, QString> kIcons = {
        {u"professional"_s, u"briefcase"_s},
        {u"casual"_s, u"coffee"_s},
        {u"polite"_s, u"hand-heart"_s},
        {u"friendly"_s, u"smile"_s},
        {u"confident"_s, u"badge-check"_s},
        {u"concise"_s, u"shrink"_s},
        {u"elaborate"_s, u"enlarge"_s},
        {u"social"_s, u"megaphone"_s},
        {u"emojify"_s, u"smile-plus"_s},
        {u"bullets"_s, u"list"_s},
        {u"table"_s, u"table"_s},
        {u"email"_s, u"mail"_s},
    };
    return kIcons.value(preset);
}

QIcon menuIcon(const QString& name, const Tokens& t)
{
    const QIcon glyph = Theme::icon(name, t.muted, kGlyph);
    if (glyph.isNull()) {
        return {};
    }
    return drawn([&](QPainter& p) {
        constexpr int kOffset = (kMenuIconSize - kGlyph) / 2;
        glyph.paint(&p, QRect(kOffset, kOffset, kGlyph, kGlyph));
    });
}

namespace {

struct Script
{
    QString glyph;  ///< a letter of the script it is written in
    QString native; ///< its name in its own writing, "" when the same as in English
};

/// The menu's languages (ui/core/languages.cpp) and a few more the detector names. Written
/// out, since the locale data names regional forms ("Español de España", "简体中文").
const QHash<QString, Script>& scripts()
{
    static const QHash<QString, Script> kScripts = {
        {u"en"_s, {.glyph = u"Aa"_s, .native = {}}},
        {u"hi"_s, {.glyph = u"अ"_s, .native = u"हिन्दी"_s}},
        {u"hinglish"_s, {.glyph = u"अa"_s, .native = {}}},
        {u"mr"_s, {.glyph = u"म"_s, .native = u"मराठी"_s}},
        {u"ne"_s, {.glyph = u"न"_s, .native = u"नेपाली"_s}},
        {u"bn"_s, {.glyph = u"অ"_s, .native = u"বাংলা"_s}},
        {u"ta"_s, {.glyph = u"த"_s, .native = u"தமிழ்"_s}},
        {u"te"_s, {.glyph = u"అ"_s, .native = u"తెలుగు"_s}},
        {u"kn"_s, {.glyph = u"ಕ"_s, .native = u"ಕನ್ನಡ"_s}},
        {u"ml"_s, {.glyph = u"മ"_s, .native = u"മലയാളം"_s}},
        {u"gu"_s, {.glyph = u"અ"_s, .native = u"ગુજરાતી"_s}},
        {u"pa"_s, {.glyph = u"ਪ"_s, .native = u"ਪੰਜਾਬੀ"_s}},
        {u"ur"_s, {.glyph = u"ا"_s, .native = u"اردو"_s}},
        {u"ar"_s, {.glyph = u"ع"_s, .native = u"العربية"_s}},
        {u"es"_s, {.glyph = u"Aa"_s, .native = u"Español"_s}},
        {u"fr"_s, {.glyph = u"Aa"_s, .native = u"Français"_s}},
        {u"de"_s, {.glyph = u"Aa"_s, .native = u"Deutsch"_s}},
        {u"pt"_s, {.glyph = u"Aa"_s, .native = u"Português"_s}},
        {u"it"_s, {.glyph = u"Aa"_s, .native = u"Italiano"_s}},
        {u"id"_s, {.glyph = u"Aa"_s, .native = u"Bahasa Indonesia"_s}},
        {u"tr"_s, {.glyph = u"Aa"_s, .native = u"Türkçe"_s}},
        {u"nl"_s, {.glyph = u"Aa"_s, .native = u"Nederlands"_s}},
        {u"zh"_s, {.glyph = u"中"_s, .native = u"中文"_s}},
        {u"ja"_s, {.glyph = u"あ"_s, .native = u"日本語"_s}},
        {u"ko"_s, {.glyph = u"한"_s, .native = u"한국어"_s}},
        {u"ru"_s, {.glyph = u"Я"_s, .native = u"Русский"_s}},
    };
    return kScripts;
}

} // namespace

QIcon scriptBadge(const QString& code, const Tokens& t)
{
    // Latin-script languages share "Aa"; a language is never shown as a flag.
    const QString glyph = scripts().value(code, {.glyph = u"Aa"_s, .native = {}}).glyph;
    return badge(glyph, QRectF(0, 1, kMenuIconSize, kMenuIconSize - 2), 4, t.hover, t.text, 12,
                 QFont::Medium);
}

QString nativeLanguageName(const QString& code)
{
    if (code == u"hinglish"_s) {
        return QCoreApplication::translate("paraphrase::app::ActionBar", "Hindi in Latin letters");
    }
    return scripts().value(code).native;
}

QIcon letterBadge(const QString& name, const Tokens& t)
{
    const QString letter = name.trimmed().left(1).toUpper();
    return badge(letter, QRectF(1, 1, kMenuIconSize - 2, kMenuIconSize - 2), (kMenuIconSize - 2) / 2.0,
                 t.accentSoft, t.accent, 10, QFont::Bold);
}

void MenuIconStyle::polish(QWidget* widget)
{
    // A see-through, frameless menu window, so the style sheet's border-radius shows; the
    // desktop's own shadow would stay square around it.
    QWidget* popup = nullptr;
    if (qobject_cast<QMenu*>(widget) != nullptr) {
        popup = widget;
    } else if (auto* combo = qobject_cast<QComboBox*>(widget)) {
        popup = combo->view()->window(); // the drop-down list's own window
        if (combo->itemDelegate()->metaObject() != &QStyledItemDelegate::staticMetaObject) {
            // Qt's own combo delegate ignores the style sheet's ::item rules.
            combo->setItemDelegate(new QStyledItemDelegate(combo));
        }
    }
    if (popup != nullptr && !popup->testAttribute(Qt::WA_TranslucentBackground)) {
        popup->setAttribute(Qt::WA_TranslucentBackground);
        popup->setWindowFlags(popup->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    }
    QProxyStyle::polish(widget);
}

int MenuIconStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
    if (metric == PM_SmallIconSize && qobject_cast<const QMenu*>(widget) != nullptr) {
        return kMenuIconSize;
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
}

NativeNamesMenu::NativeNamesMenu(QWidget* parent)
    : QMenu(parent)
{}

void NativeNamesMenu::setNativeName(QAction* action, const QString& native)
{
    action->setProperty("nativeName", native);
    // Room for the widest name pair: QMenu sizes itself from the labels alone.
    QFont small = font();
    small.setPixelSize(kNativePx);
    const QFontMetrics label(font());
    const QFontMetrics side(small);
    const int width = kMenuIconSize + label.horizontalAdvance(action->text()) + kNativeGap +
                      side.horizontalAdvance(native) + (3 * kNativeRightPad);
    setMinimumWidth(std::max(minimumWidth(), width));
}

void NativeNamesMenu::addCaption(const QString& text)
{
    QAction* caption = addAction(QString());
    caption->setEnabled(false);
    caption->setProperty("caption", text);
}

void NativeNamesMenu::paintEvent(QPaintEvent* event)
{
    QMenu::paintEvent(event);
    QPainter p(this);
    QFont caps = font();
    caps.setPixelSize(11);
    caps.setWeight(QFont::Bold);
    caps.setCapitalization(QFont::AllUppercase);
    caps.setLetterSpacing(QFont::PercentageSpacing, 106); // .cap { letter-spacing: .06em }
    p.setPen(m_muted);
    for (QAction* a : actions()) {
        if (const QString caption = a->property("caption").toString(); !caption.isEmpty()) {
            p.setFont(caps);
            p.drawText(actionGeometry(a).adjusted(10, 4, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, caption);
        }
    }
    QFont small = font();
    small.setPixelSize(kNativePx);
    p.setFont(small);
    p.setPen(m_muted);
    for (QAction* a : actions()) {
        const QString native = a->property("nativeName").toString();
        if (!native.isEmpty()) {
            p.drawText(actionGeometry(a).adjusted(0, 0, -kNativeRightPad, 0),
                       Qt::AlignRight | Qt::AlignVCenter, native);
        }
    }
}

} // namespace paraphrase::app
