#include "models_settings_page.h"

#include "installed_models_table.h"
#include "languages_section.h"
#include "models_controller.h"
#include "tier_pickers.h"
#include "widgets.h"
#include "window_style.h"

#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

QFrame* card(QVBoxLayout*& col)
{
    auto* frame = new QFrame;
    frame->setObjectName(u"card"_s);
    col = new QVBoxLayout(frame);
    col->setContentsMargins(18, 16, 18, 16);
    col->setSpacing(10);
    return frame;
}

} // namespace

ModelsSettingsPage::ModelsSettingsPage(ModelsController& models, QWidget* parent)
    : QWidget(parent)
    , m_models(models)
    , m_tiers(new TierPickers(models))
    , m_table(new InstalledModelsTable(models))
{
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 4, 0);
    col->setSpacing(18);
    col->addWidget(makeLabel(tr("Models"), u"h1"_s));
    QLabel* intro =
        makeLabel(tr("Paraphrase runs these models on this computer. Quick actions use the fast model; "
                     "tone changes and longer writing use the quality model when it is installed."),
                  u"muted"_s);
    intro->setWordWrap(true);
    intro->setMaximumWidth(560);
    col->addWidget(intro);
    QVBoxLayout* tiersCol = nullptr;
    col->addWidget(card(tiersCol));
    tiersCol->addWidget(makeLabel(tr("Which model does what"), u"h2"_s));
    tiersCol->addWidget(m_tiers);
    col->addWidget(buildInstalledCard());
    col->addWidget(new LanguagesSection);
    col->addWidget(buildFolderLine());
    col->addStretch();
    connect(m_tiers, &TierPickers::problem, this, &ModelsSettingsPage::showProblem);
    connect(m_table, &InstalledModelsTable::problem, this, &ModelsSettingsPage::showProblem);
    connect(&m_models, &ModelsController::changed, this, &ModelsSettingsPage::reload);
    connect(&m_models, &ModelsController::progress, m_table, &InstalledModelsTable::showProgress);
    connect(&m_models, &ModelsController::stateChanged, &m_models, &ModelsController::refresh);
}

QWidget* ModelsSettingsPage::buildInstalledCard()
{
    QVBoxLayout* col = nullptr;
    QFrame* frame = card(col);
    auto* head = new QHBoxLayout;
    head->setSpacing(8);
    head->addWidget(makeLabel(tr("Installed"), u"h2"_s));
    m_diskUse = makeLabel(QString(), u"small"_s);
    head->addWidget(m_diskUse);
    head->addStretch();
    m_add = makeButton(tr("+  Add a model"));
    m_add->setObjectName(u"addModel"_s);
    m_addMenu = new QMenu(m_add);
    m_add->setMenu(m_addMenu);
    head->addWidget(m_add);
    col->addLayout(head);
    col->addWidget(m_table);
    m_problem = makeLabel(QString(), u"danger"_s);
    m_problem->setObjectName(u"modelsProblem"_s);
    m_problem->setWordWrap(true);
    m_problem->hide();
    col->addWidget(m_problem);
    return frame;
}

QWidget* ModelsSettingsPage::buildFolderLine()
{
    // Where the models live, the real folder (note 4).
    auto* line = new QWidget;
    auto* row = new QHBoxLayout(line);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(10);
    m_lock = new QLabel;
    row->addWidget(m_lock, 0, Qt::AlignTop);
    m_folder = makeLabel(QString());
    m_folder->setObjectName(u"modelsFolder"_s);
    m_folder->setTextFormat(Qt::RichText);
    m_folder->setWordWrap(true);
    m_folder->setStyleSheet(u"font-size: 12px;"_s);
    connect(m_folder, &QLabel::linkActivated, this,
            [this] { QDesktopServices::openUrl(QUrl::fromLocalFile(m_models.modelsDir())); });
    row->addWidget(m_folder, 1);
    return line;
}

void ModelsSettingsPage::setTheme(const Tokens& tokens)
{
    m_tokens = tokens;
    m_lock->setPixmap(Theme::icon(u"lock"_s, tokens.privacy, 14).pixmap(14, 14));
    reload();
}

void ModelsSettingsPage::reload()
{
    m_tiers->reload();
    m_table->reload();
    fillAddMenu();
    qint64 onDisk = 0;
    for (const ModelInfo& m : m_models.models()) {
        onDisk += m.installed ? m.size : 0;
    }
    m_diskUse->setText(tr("%1 used on disk").arg(formatSize(onDisk)));
    m_folder->setText(
        tr("Models download only from their makers, at a fixed version, and are checked before use. "
           "They live in <span style=\"font-family: monospace\">%1</span> %2")
            .arg(m_models.modelsDir().toHtmlEscaped(), link(u"open"_s, tr("Open folder"), m_tokens)));
}

void ModelsSettingsPage::fillAddMenu()
{
    m_addMenu->clear();
    for (const ModelInfo& m : m_models.models()) {
        if (!m.isModel() || m.installed || !m.downloadState.isEmpty()) {
            continue;
        }
        QString text = tr("%1 · %2 download").arg(m.name, formatSize(m.size));
        if (!m.fits) {
            text += tr(" · too large for this computer");
        }
        QAction* a = m_addMenu->addAction(text);
        a->setObjectName(u"add-"_s + m.id);
        a->setEnabled(m.fits);
        connect(a, &QAction::triggered, this, [this, id = m.id] { showProblem(m_models.download(id)); });
    }
    m_add->setEnabled(!m_addMenu->isEmpty());
}

void ModelsSettingsPage::showProblem(const QString& message)
{
    m_problem->setText(message);
    m_problem->setVisible(!message.isEmpty());
}

} // namespace paraphrase::app
