#include "ui/parts/tone_picker.h"

#include "ui/common/menu_icons.h"
#include "ui/common/widgets.h"

#include <QMenu>
#include <QSettings>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {
constexpr int kIconSize = 14;
constexpr int kIconGap = 4; // as setButtonIcon pads it: the mocks' 6 px gap
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
    reserveWidth(this, m_titles); // Friendly or Professional: the button keeps one width
}

void TonePicker::choose(const QString& tone, const QString& title)
{
    m_tone = tone;
    setText(title);
    showIcon();
}

void TonePicker::showIcon()
{
    // The chosen tone's icon before its name; the menu arrow after it is the style sheet's
    // (QPushButton#toneChoice::menu-indicator), so a tone looks the same here and in the menu.
    setIcon(Theme::icon(presetIcon(m_tone), m_tokens.muted, kIconSize, 0, kIconGap));
    setIconSize(QSize(kIconSize + kIconGap, kIconSize));
}

void TonePicker::applyTheme(const Tokens& t)
{
    m_tokens = t;
    showIcon();
    reserveWidth(this, m_titles);
}

} // namespace paraphrase::app
