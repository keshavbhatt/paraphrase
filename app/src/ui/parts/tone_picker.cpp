#include "ui/parts/tone_picker.h"

#include "ui/common/menu_icons.h"

#include <QMenu>
#include <QPainter>
#include <QSettings>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {
// mock.css .btn: 14 px padding, 6 px between the parts, 14 px icons (.ic.s).
constexpr int kPad = 14;
constexpr int kGap = 6;
constexpr int kIconSize = 14;
} // namespace

TonePicker::TonePicker(QString settingsKey, QString fallback, QWidget* parent)
    : QPushButton(parent)
    , m_key(std::move(settingsKey))
    , m_fallback(std::move(fallback))
{
    setObjectName(u"toneChoice"_s);
    setCursor(Qt::PointingHandCursor);
    setMenu(new QMenu(this));
    // Each tone's icon when it opens, in the theme's colours then (menus.html).
    connect(menu(), &QMenu::aboutToShow, this, [this] {
        for (QAction* item : menu()->actions()) {
            item->setIcon(menuIcon(presetIcon(item->objectName().section(u'-', 1)), m_tokens));
        }
    });
}

void TonePicker::setTones(const QList<std::pair<QString, QString>>& tones)
{
    menu()->clear();
    const QString saved = QSettings().value(m_key, m_fallback).toString();
    m_tone.clear();
    m_titles.clear();
    for (const auto& [tone, title] : tones) {
        m_titles.append(title);
        QAction* item = menu()->addAction(title);
        item->setObjectName(u"tone-"_s + tone);
        connect(item, &QAction::triggered, this, [this, tone, title] {
            const bool other = tone != m_tone;
            choose(tone, title);
            QSettings().setValue(m_key, tone);
            if (other) {
                Q_EMIT changed();
            }
        });
        if (tone == saved || m_tone.isEmpty()) {
            choose(tone, title);
        }
    }
    setVisible(!tones.isEmpty());
    updateGeometry(); // Friendly or Professional: the button keeps the widest one's width
}

void TonePicker::choose(const QString& tone, const QString& title)
{
    m_tone = tone;
    m_title = title;
    setAccessibleName(tr("Tone: %1").arg(title));
    update();
}

void TonePicker::applyTheme(const Tokens& t)
{
    m_tokens = t;
    update();
}

QSize TonePicker::sizeHint() const
{
    // The tone's icon, its name and the menu arrow (menus.html), as wide as the widest tone,
    // so picking another never moves what is beside it.
    const QFontMetrics fm(font());
    int widest = 0;
    for (const QString& title : m_titles) {
        widest = std::max(widest, fm.horizontalAdvance(title));
    }
    const int width = kPad + kIconSize + kGap + widest + kGap + kIconSize + kPad;
    return {width, QPushButton::sizeHint().height()};
}

void TonePicker::paintEvent(QPaintEvent* event)
{
    // The style sheet draws the frame; Qt cannot place an icon and a menu arrow on one
    // button reliably, so the parts are drawn here.
    QPushButton::paintEvent(event);
    QPainter p(this);
    const int mid = height() / 2;
    const QColor ink = isEnabled() ? m_tokens.text : m_tokens.muted;
    const QIcon icon = Theme::icon(presetIcon(m_tone), m_tokens.muted, kIconSize);
    icon.paint(&p, QRect(kPad, mid - (kIconSize / 2), kIconSize, kIconSize));
    const int textLeft = kPad + kIconSize + kGap;
    const QRect textBox(textLeft, 0, width() - textLeft - kGap - kIconSize - kPad, height());
    p.setPen(ink);
    p.setFont(font());
    p.drawText(textBox, Qt::AlignLeft | Qt::AlignVCenter, m_title);
    const QIcon arrow = Theme::icon(u"down"_s, m_tokens.muted, kIconSize);
    arrow.paint(&p, QRect(width() - kPad - kIconSize, mid - (kIconSize / 2), kIconSize, kIconSize));
}

} // namespace paraphrase::app
