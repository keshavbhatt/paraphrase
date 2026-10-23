#pragma once

#include "ui/common/theme.h"

#include <QList>
#include <QPushButton>
#include <QString>
#include <QStringList>

#include <utility>

namespace paraphrase::app {

/// A tone to write in, chosen from the Tone action's presets and remembered under a settings
/// key (the full reply and Write each keep their own).
class TonePicker : public QPushButton
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TonePicker)

public:
    TonePicker(QString settingsKey, QString fallback, QWidget* parent = nullptr);
    ~TonePicker() override = default;

    /// The tones to offer, as (tone, title).
    void setTones(const QList<std::pair<QString, QString>>& tones);
    [[nodiscard]] const QString& tone() const { return m_tone; }
    void applyTheme(const Tokens& t);

Q_SIGNALS:
    void changed(); ///< the user picked another tone

private:
    void choose(const QString& tone, const QString& title);
    void showIcon();

    QString m_key;
    QString m_fallback;
    QString m_tone;
    QStringList m_titles; ///< every tone's title, so the button can keep the widest one's width
    Tokens m_tokens;
};

} // namespace paraphrase::app
