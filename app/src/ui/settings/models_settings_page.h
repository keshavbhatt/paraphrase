#pragma once

#include "ui/common/theme.h"

#include <QWidget>

class QLabel;
class QMenu;
class QPushButton;

namespace paraphrase::app {

class InstalledModelsTable;
class ModelsController;
class TierPickers;

/// Settings, Models (settings-models.html): the tier pickers, the Installed card with
/// "Add a model", the languages and where the models live.
class ModelsSettingsPage : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ModelsSettingsPage)

public:
    explicit ModelsSettingsPage(ModelsController& models, QWidget* parent = nullptr);
    ~ModelsSettingsPage() override = default;

    /// Icons and link colours for the current theme.
    void setTheme(const Tokens& tokens);

private:
    [[nodiscard]] QWidget* buildInstalledCard();
    [[nodiscard]] QWidget* buildFolderLine();
    void reload();
    void fillAddMenu();
    void showProblem(const QString& message);

    ModelsController& m_models;
    TierPickers* m_tiers;
    InstalledModelsTable* m_table;
    QLabel* m_diskUse = nullptr;
    QPushButton* m_add = nullptr;
    QMenu* m_addMenu = nullptr;
    QLabel* m_problem = nullptr;
    QLabel* m_folder = nullptr;
    QLabel* m_lock = nullptr;
    Tokens m_tokens;
};

} // namespace paraphrase::app
