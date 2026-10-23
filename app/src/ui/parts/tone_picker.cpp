#include "ui/parts/tone_picker.h"

#include "ui/common/widgets.h"

#include <QMenu>
#include <QSettings>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

TonePicker::TonePicker(QString settingsKey, QString fallback, QWidget* parent)
    : QPushButton(parent)
    , m_key(std::move(settingsKey))
    , m_fallback(std::move(fallback))
{
    setObjectName(u"toneChoice"_s);
    setCursor(Qt::PointingHandCursor);
    setMenu(new QMenu(this));
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
            choose(tone, title);
            QSettings().setValue(m_key, tone);
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
}

void TonePicker::applyTheme(const Tokens& t)
{
    // The menu arrow after the tone's name, as on the actions.
    setLayoutDirection(Qt::RightToLeft);
    setIcon(Theme::icon(u"down"_s, t.muted, 14));
    reserveWidth(this, m_titles);
}

} // namespace paraphrase::app
