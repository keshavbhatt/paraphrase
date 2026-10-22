#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QWidget>

class QHBoxLayout;
class QLabel;
class QVBoxLayout;

namespace paraphrase::app {

class DownloadPanel;
class HardwareChips;
class ModelCard;
class ModelsController;

/// The setup window's model step (setup-model.html): the computer, a card per tier, a download
/// panel for each ticked model, the privacy line. Downloads start only on a click.
class ModelPage : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ModelPage)

public:
    explicit ModelPage(ModelsController& models, QWidget* parent = nullptr);
    ~ModelPage() override = default;

    /// Builds the cards from the controller's lists (call after refresh()).
    void load();
    /// A ticked model is installed: the user may go on.
    [[nodiscard]] bool ready() const;
    void setLockIcon(const QPixmap& icon);

Q_SIGNALS:
    void readyChanged(bool ready);

private:
    void onToggled(const QString& modelId, bool on);
    void onAction(const QString& modelId);
    void updatePanel(const QString& modelId);
    void showPanel(const QString& modelId);

    ModelsController& m_models;
    HardwareChips* m_chips = nullptr;
    QHBoxLayout* m_cardsRow = nullptr;
    QVBoxLayout* m_panelsBox = nullptr;
    QLabel* m_lock = nullptr;
    QList<ModelCard*> m_cards;
    QHash<QString, DownloadPanel*> m_panels;
};

} // namespace paraphrase::app
