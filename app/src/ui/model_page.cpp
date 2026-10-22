#include "model_page.h"

#include "download_panel.h"
#include "hardware_chips.h"
#include "model_card.h"
#include "models_controller.h"
#include "window_style.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

ModelPage::ModelPage(ModelsController& models, QWidget* parent)
    : QWidget(parent)
    , m_models(models)
    , m_chips(new HardwareChips)
    , m_cardsRow(new QHBoxLayout)
    , m_panelsBox(new QVBoxLayout)
    , m_lock(new QLabel)
{
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(24, 22, 24, 22);
    col->setSpacing(16);
    auto* head = new QVBoxLayout;
    head->setSpacing(8);
    head->addWidget(textLabel(tr("Choose the model that writes for you"), u"h1"_s));
    head->addWidget(m_chips);
    col->addLayout(head);
    m_cardsRow->setSpacing(14);
    col->addLayout(m_cardsRow);
    m_panelsBox->setSpacing(10);
    col->addLayout(m_panelsBox);
    auto* privacy = new QHBoxLayout;
    privacy->setSpacing(10);
    privacy->addWidget(m_lock, 0, Qt::AlignTop);
    QLabel* promise = textLabel(tr("Downloading models is the only time Paraphrase goes online. What you write "
                                   "never leaves this computer, and there is no account."));
    promise->setStyleSheet(u"font-size: 12px;"_s);
    promise->setWordWrap(true);
    privacy->addWidget(promise, 1);
    col->addLayout(privacy);
    col->addStretch();
    connect(&m_models, &ModelsController::progress, this, &ModelPage::updatePanel);
    connect(&m_models, &ModelsController::stateChanged, this, [this](const QString& id) {
        updatePanel(id);
        Q_EMIT readyChanged(ready());
    });
}

void ModelPage::setLockIcon(const QPixmap& icon)
{
    m_lock->setPixmap(icon);
}

void ModelPage::load()
{
    m_chips->setComputer(m_models.computer());
    const std::pair<QString, ModelCard::Tier> tiers[] = {
        {u"fast"_s, ModelCard::Tier::Fast},
        {u"quality"_s, ModelCard::Tier::Quality},
    };
    for (const auto& [tier, kind] : tiers) {
        const ModelInfo* m = m_models.offerFor(tier);
        if (m == nullptr) {
            continue;
        }
        auto* card = new ModelCard(*m, kind);
        m_cardsRow->addWidget(card, 1);
        m_cards.append(card);
        connect(card, &ModelCard::toggled, this, &ModelPage::onToggled);
        // Fast is preselected (note 2); a model already on its way stays ticked.
        const bool tick = (kind == ModelCard::Tier::Fast && m->fits) || m->installed || !m->downloadState.isEmpty();
        card->setChecked(tick);
    }
    Q_EMIT readyChanged(ready());
}

bool ModelPage::ready() const
{
    for (const ModelCard* card : m_cards) {
        const ModelInfo* m = m_models.model(card->modelId());
        if (card->isChecked() && m != nullptr && m->installed) {
            return true;
        }
    }
    return false;
}

void ModelPage::onToggled(const QString& modelId, bool on)
{
    if (on) {
        showPanel(modelId);
    } else if (DownloadPanel* panel = m_panels.take(modelId); panel != nullptr) {
        // Unticking pauses a running download and keeps what arrived; nothing is deleted here.
        const ModelInfo* m = m_models.model(modelId);
        if (m != nullptr && (m->downloadState == u"downloading"_s || m->downloadState == u"waiting"_s)) {
            (void)m_models.pause(modelId);
        }
        panel->deleteLater();
    }
    Q_EMIT readyChanged(ready());
}

void ModelPage::showPanel(const QString& modelId)
{
    const ModelInfo* m = m_models.model(modelId);
    if (m == nullptr || m_panels.contains(modelId)) {
        return;
    }
    auto* panel = new DownloadPanel(*m);
    connect(panel, &DownloadPanel::actionClicked, this, &ModelPage::onAction);
    m_panels.insert(modelId, panel);
    m_panelsBox->addWidget(panel);
}

void ModelPage::onAction(const QString& modelId)
{
    const ModelInfo* found = m_models.model(modelId);
    if (found == nullptr) {
        return;
    }
    const ModelInfo before = *found; // the calls below read the lists again
    const bool running = before.downloadState == u"downloading"_s || before.downloadState == u"waiting"_s;
    // Download, Resume and Retry: the user asked for it with this click.
    if (const QString problem = running ? m_models.pause(modelId) : m_models.download(modelId); !problem.isEmpty()) {
        ModelInfo failed = before;
        failed.downloadState = u"failed"_s;
        failed.downloadError = problem;
        m_panels.value(modelId)->update(failed, 0);
        return;
    }
    updatePanel(modelId);
}

void ModelPage::updatePanel(const QString& modelId)
{
    DownloadPanel* panel = m_panels.value(modelId);
    const ModelInfo* m = m_models.model(modelId);
    if (panel != nullptr && m != nullptr) {
        panel->update(*m, m_models.speed(modelId));
    }
}

} // namespace paraphrase::app
