#pragma once

#include "ui/models/model_info.h"

#include <QFrame>
#include <QString>

class QCheckBox;

namespace paraphrase::app {

/// One tier's model to tick on the setup page (setup-model.html, note 2): Fast is
/// recommended, Quality optional; a model too large for this computer cannot be ticked.
class ModelCard : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ModelCard)

public:
    enum class Tier : std::uint8_t
    {
        Fast,
        Quality,
    };

    ModelCard(const ModelInfo& model, Tier tier, QWidget* parent = nullptr);
    ~ModelCard() override = default;

    [[nodiscard]] QString modelId() const { return m_modelId; }
    [[nodiscard]] bool isChecked() const;
    void setChecked(bool on);

Q_SIGNALS:
    void toggled(const QString& modelId, bool on);

protected:
    void mouseReleaseEvent(QMouseEvent* event) override; ///< the whole card ticks, as a <label>

private:
    QString m_modelId;
    QCheckBox* m_tick = nullptr;
};

} // namespace paraphrase::app
