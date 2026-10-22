#include "model_card.h"

#include "window_style.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

QString describe(const ModelInfo& m, ModelCard::Tier tier)
{
    if (!m.fits) {
        return ModelCard::tr("Too large for this computer's memory.");
    }
    return tier == ModelCard::Tier::Fast
               ? ModelCard::tr("Proofread, reply and quick fixes. Works on 8 GB computers.")
               : ModelCard::tr("Better tone changes and longer writing. Best with 16 GB memory.");
}

QString facts(const ModelInfo& m)
{
    QString line = ModelCard::tr("%1 download · %2 memory in use").arg(formatSize(m.size), formatMemory(m.memoryGb));
    if (m.installed) {
        line += ModelCard::tr(" · installed");
    }
    return line;
}

} // namespace

ModelCard::ModelCard(const ModelInfo& model, Tier tier, QWidget* parent)
    : QFrame(parent)
    , m_modelId(model.id)
    , m_tick(new QCheckBox)
{
    setObjectName(u"modelCard"_s);
    setCursor(Qt::PointingHandCursor);
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(16, 16, 16, 16);
    col->setSpacing(8);
    auto* top = new QHBoxLayout;
    top->setSpacing(8);
    m_tick->setObjectName(u"tick-"_s + model.id);
    m_tick->setAccessibleName(model.name);
    m_tick->setEnabled(model.fits);
    top->addWidget(m_tick);
    QLabel* name = textLabel(tier == Tier::Fast ? tr("Fast") : tr("Quality"));
    name->setStyleSheet(u"font-size: 15px; font-weight: 600;"_s);
    top->addWidget(name);
    top->addWidget(tier == Tier::Fast ? textLabel(tr("Recommended"), u"pill-accent"_s)
                                      : textLabel(tr("Optional"), u"pill"_s));
    top->addStretch();
    col->addLayout(top);
    col->addWidget(textLabel(model.name));
    QLabel* about = textLabel(describe(model, tier), u"small"_s);
    about->setWordWrap(true);
    col->addWidget(about);
    QLabel* line = textLabel(facts(model));
    line->setStyleSheet(u"font-size: 12px;"_s);
    col->addWidget(line);
    col->addStretch();
    connect(m_tick, &QCheckBox::toggled, this, [this](bool on) {
        setProperty("on", on);
        repolish(this);
        Q_EMIT toggled(m_modelId, on);
    });
}

bool ModelCard::isChecked() const
{
    return m_tick->isChecked();
}

void ModelCard::setChecked(bool on)
{
    m_tick->setChecked(on);
    setProperty("on", on);
    repolish(this);
}

void ModelCard::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_tick->isEnabled() && event->button() == Qt::LeftButton) {
        m_tick->toggle();
    }
    QFrame::mouseReleaseEvent(event);
}

} // namespace paraphrase::app
