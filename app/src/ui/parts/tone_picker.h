#pragma once

#include "ui/common/theme.h"

#include <QList>
#include <QPushButton>
#include <QString>

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

private:
    void choose(const QString& tone, const QString& title);

    QString m_key;
    QString m_fallback;
    QString m_tone;
};

} // namespace paraphrase::app
