#include "ui/settings/installed_models_table.h"

#include "ui/common/widgets.h"
#include "ui/common/window_style.h"
#include "ui/models/models_controller.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

enum Column : std::uint8_t
{
    Name,
    Use,
    Size,
    Memory,
    State,
    Action,
    Columns,
};

int percentOf(const ModelInfo& m)
{
    const qint64 total = m.total > 0 ? m.total : m.size;
    return total > 0 ? static_cast<int>(m.received * 100 / total) : 0;
}

bool isRunning(const ModelInfo& m)
{
    return m.downloadState == u"downloading"_s || m.downloadState == u"waiting"_s;
}

} // namespace

QString downloadStateText(const ModelInfo& m)
{
    if (m.downloadState == u"paused"_s) {
        return InstalledModelsTable::tr("Paused at %1%").arg(percentOf(m));
    }
    if (m.downloadState == u"failed"_s) {
        return InstalledModelsTable::tr("Did not download");
    }
    if (m.downloadState == u"verifying"_s) {
        return InstalledModelsTable::tr("Checking");
    }
    return InstalledModelsTable::tr("Downloading %1%").arg(percentOf(m));
}

InstalledModelsTable::InstalledModelsTable(ModelsController& models, QWidget* parent)
    : QWidget(parent)
    , m_models(models)
    , m_grid(new QGridLayout(this))
{
    setObjectName(u"installedModels"_s);
    m_grid->setContentsMargins(10, 0, 10, 0); // table.list cells: 10 px in from the card
    m_grid->setHorizontalSpacing(20);
    m_grid->setVerticalSpacing(0);
    m_grid->setColumnStretch(Name, 3);
    m_grid->setColumnStretch(Use, 1);
}

void InstalledModelsTable::reload()
{
    while (const QLayoutItem* item = m_grid->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    addHeader();
    int row = 1;
    for (const ModelInfo& m : m_models.models()) {
        if (m.isModel() && (m.installed || !m.downloadState.isEmpty())) {
            addModelRow(row++, m);
        }
    }
    addSpeedUpRow(row);
}

void InstalledModelsTable::showProgress(const QString& modelId)
{
    const ModelInfo* m = m_models.model(modelId);
    if (auto* state = findChild<QLabel*>(u"state-"_s + modelId); state != nullptr && m != nullptr) {
        state->setText(downloadStateText(*m));
    }
}

void InstalledModelsTable::addHeader()
{
    const QStringList headers = {tr("MODEL"), tr("USED FOR"), tr("SIZE"), tr("MEMORY"), tr("STATUS")};
    for (int c = 0; c < headers.size(); ++c) {
        QLabel* h = makeLabel(headers.at(c), u"th"_s);
        h->setContentsMargins(0, 8, 0, 8);
        m_grid->addWidget(h, 0, c, c == Size ? Qt::AlignRight : Qt::AlignLeft);
    }
}

void InstalledModelsTable::addRule(int row)
{
    auto* rule = new QFrame;
    rule->setObjectName(u"rule"_s);
    m_grid->addWidget(rule, row, 0, 1, Columns);
}

QWidget* InstalledModelsTable::nameCell(const QString& name, const QString& detail)
{
    auto* cell = new QWidget;
    auto* col = new QVBoxLayout(cell);
    col->setContentsMargins(0, 10, 0, 10);
    col->setSpacing(1);
    QLabel* title = makeLabel(name);
    title->setProperty("role", u"strong"_s); // not a widget style sheet: it clipped descenders
    // Both wrap: unwrapped, the speed-up's line made the table 866 px wide, more than the
    // window gives it at its opening size, and the cards were cut off at the right.
    title->setWordWrap(true);
    col->addWidget(title);
    QLabel* line = makeLabel(detail, u"small"_s);
    line->setWordWrap(true);
    col->addWidget(line);
    return cell;
}

void InstalledModelsTable::addModelRow(int row, const ModelInfo& m)
{
    const int r = row * 2; // each row has a rule above it
    addRule(r - 1);
    m_grid->addWidget(nameCell(m.name, tr("%1 · %2").arg(m.maker(), m.licence)), r, Name);
    m_grid->addWidget(makeLabel(usedFor(m.id)), r, Use);
    m_grid->addWidget(makeLabel(formatSize(m.size)), r, Size, Qt::AlignRight);
    m_grid->addWidget(makeLabel(formatMemory(m.memoryGb)), r, Memory);
    QString pill = u"pill"_s;
    if (m.installed) {
        pill = u"pill-ok"_s;
    } else if (m.downloadState == u"failed"_s) {
        pill = u"pill-warn"_s;
    }
    QLabel* state = makeLabel(m.installed ? tr("Ready, verified") : downloadStateText(m), pill);
    state->setObjectName(u"state-"_s + m.id);
    m_grid->addWidget(state, r, State, Qt::AlignLeft | Qt::AlignVCenter);
    m_grid->addWidget(m_confirming == m.id ? confirmCell(m) : actionCell(m), r, Action, Qt::AlignRight);
}

QWidget* InstalledModelsTable::actionCell(const ModelInfo& m)
{
    const QString id = m.id;
    if (!m.installed) {
        const bool running = isRunning(m);
        QPushButton* b = makeButton(running ? tr("Pause") : tr("Resume"), u"link"_s);
        b->setObjectName(u"resume-"_s + id);
        connect(b, &QPushButton::clicked, this,
                [this, id, running] { act(running ? m_models.pause(id) : m_models.download(id)); });
        return b;
    }
    QPushButton* b = makeButton(tr("Remove"), u"danger"_s);
    b->setObjectName(u"remove-"_s + id);
    connect(b, &QPushButton::clicked, this, [this, id] {
        m_confirming = id;
        Q_EMIT problem({});
        reload();
    });
    return b;
}

QWidget* InstalledModelsTable::confirmCell(const ModelInfo& m)
{
    auto* cell = new QWidget;
    auto* row = new QHBoxLayout(cell);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(6);
    row->addWidget(makeLabel(tr("Remove %1?").arg(formatSize(m.size)), u"small"_s));
    QPushButton* yes = makeButton(tr("Remove"), u"dangerSolid"_s);
    yes->setObjectName(u"confirmRemove-"_s + m.id);
    QPushButton* no = makeButton(tr("Keep"));
    no->setObjectName(u"keep-"_s + m.id);
    const QString id = m.id;
    connect(yes, &QPushButton::clicked, this, [this, id] {
        m_confirming.clear();
        act(m_models.remove(id)); // the second, explicit click
    });
    connect(no, &QPushButton::clicked, this, [this] {
        m_confirming.clear();
        reload();
    });
    row->addWidget(yes);
    row->addWidget(no);
    return cell;
}

void InstalledModelsTable::addSpeedUpRow(int row)
{
    // The draft heads, listed so they are never a hidden download (note 3).
    qint64 size = 0;
    int count = 0;
    for (const ModelInfo& m : m_models.models()) {
        if (m.isDraft() && m.installed) {
            size = std::max(size, m.size);
            ++count;
        }
    }
    if (count == 0) {
        return;
    }
    const int r = row * 2;
    addRule(r - 1);
    m_grid->addWidget(
        nameCell(tr("Speed-up for each model"), tr("A small helper that drafts ahead: same results, faster")),
        r, Name);
    m_grid->addWidget(makeLabel(tr("Proofread, tone")), r, Use);
    const QString sizeText = count == 1 ? formatSize(size) : tr("%1 × %2").arg(count).arg(formatSize(size));
    m_grid->addWidget(makeLabel(sizeText), r, Size, Qt::AlignRight);
    m_grid->addWidget(makeLabel(tr("under 0.1 GB")), r, Memory);
    const bool on = m_models.speedUp();
    QLabel* state = makeLabel(on ? tr("On") : tr("Off"), on ? u"pill-ok"_s : u"pill"_s);
    state->setObjectName(u"speedUpState"_s);
    m_grid->addWidget(state, r, State, Qt::AlignLeft | Qt::AlignVCenter);
    QPushButton* toggle = makeButton(on ? tr("Turn off") : tr("Turn on"), u"link"_s);
    toggle->setObjectName(u"speedUp"_s);
    connect(toggle, &QPushButton::clicked, this, [this, on] { act(m_models.setSpeedUp(!on)); });
    m_grid->addWidget(toggle, r, Action, Qt::AlignRight);
}

QString InstalledModelsTable::usedFor(const QString& modelId) const
{
    QStringList uses;
    if (m_models.tierModel(u"fast"_s) == modelId) {
        uses.append(tr("Fast"));
    }
    if (m_models.tierModel(u"quality"_s) == modelId) {
        uses.append(tr("Quality"));
    }
    return uses.isEmpty() ? tr("Not used") : uses.join(u", "_s);
}

void InstalledModelsTable::act(const QString& problem)
{
    Q_EMIT this->problem(problem);
}

} // namespace paraphrase::app
