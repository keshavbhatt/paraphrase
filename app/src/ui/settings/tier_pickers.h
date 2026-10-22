#pragma once

#include <QWidget>

class QComboBox;

namespace paraphrase::app {

class ModelsController;

/// "Which model does what" (settings-models.html, note 1): one picker per tier among the
/// installed models. The quality tier on the fast model says so in words.
class TierPickers : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TierPickers)

public:
    explicit TierPickers(ModelsController& models, QWidget* parent = nullptr);
    ~TierPickers() override = default;

    void reload();

Q_SIGNALS:
    void problem(const QString& message);

private:
    void fill(QComboBox* combo, const QString& tier);

    ModelsController& m_models;
    QComboBox* m_fast;
    QComboBox* m_quality;
};

} // namespace paraphrase::app
