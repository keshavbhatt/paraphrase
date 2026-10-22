#pragma once

#include "model_info.h"

#include <QString>
#include <QWidget>

class QGridLayout;

namespace paraphrase::app {

class ModelsController;

/// The Installed card's table (settings-models.html, notes 2 and 3): each installed or
/// downloading model with what it is used for, size, memory and state, and the speed-up row.
/// Remove asks once, in place: the first click turns the row into "Remove 3.35 GB? Remove Keep".
class InstalledModelsTable : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(InstalledModelsTable)

public:
    explicit InstalledModelsTable(ModelsController& models, QWidget* parent = nullptr);
    ~InstalledModelsTable() override = default;

    /// Redraws from the controller.
    void reload();
    /// Updates one downloading row's percentage without a redraw.
    void showProgress(const QString& modelId);

Q_SIGNALS:
    void problem(const QString& message); ///< the daemon refused an action; "" clears it

private:
    void addHeader();
    void addRule(int row);
    void addModelRow(int row, const ModelInfo& m);
    void addSpeedUpRow(int row);
    [[nodiscard]] static QWidget* nameCell(const QString& name, const QString& detail);
    [[nodiscard]] QWidget* actionCell(const ModelInfo& m);
    [[nodiscard]] QWidget* confirmCell(const ModelInfo& m);
    [[nodiscard]] QString usedFor(const QString& modelId) const;
    void act(const QString& problem);

    ModelsController& m_models;
    QGridLayout* m_grid;
    QString m_confirming; ///< the model whose Remove waits for the second click
};

/// What a downloading model's State cell says: "Downloading 36%", "Paused at 36%", ...
[[nodiscard]] QString downloadStateText(const ModelInfo& m);

} // namespace paraphrase::app
