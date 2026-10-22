#include "tier_pickers.h"

#include "models_controller.h"
#include "widgets.h"
#include "window_style.h"

#include <QComboBox>
#include <QGridLayout>
#include <QLabel>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

TierPickers::TierPickers(ModelsController& models, QWidget* parent)
    : QWidget(parent)
    , m_models(models)
    , m_fast(new QComboBox)
    , m_quality(new QComboBox)
{
    auto* grid = new QGridLayout(this);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(22);
    grid->setVerticalSpacing(6);
    grid->addWidget(makeLabel(tr("Fast: proofread, reply, format"), u"small"_s), 0, 0);
    grid->addWidget(makeLabel(tr("Quality: tone, summarize, translate, compose"), u"small"_s), 0, 1);
    m_fast->setObjectName(u"tierFast"_s);
    m_quality->setObjectName(u"tierQuality"_s);
    grid->addWidget(m_fast, 1, 0);
    grid->addWidget(m_quality, 1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    const std::pair<QComboBox*, QString> pickers[] = {{m_fast, u"fast"_s}, {m_quality, u"quality"_s}};
    for (const auto& [combo, tier] : pickers) {
        connect(combo, &QComboBox::activated, this, [this, c = combo, t = tier](int index) {
            Q_EMIT problem(m_models.setTierModel(t, c->itemData(index).toString()));
        });
    }
}

void TierPickers::reload()
{
    fill(m_fast, u"fast"_s);
    fill(m_quality, u"quality"_s);
}

void TierPickers::fill(QComboBox* combo, const QString& tier)
{
    combo->clear();
    const QString current = m_models.tierModel(tier);
    for (const ModelInfo& m : m_models.models()) {
        if (!m.isModel() || !m.installed) {
            continue;
        }
        QString text = m.name;
        if (tier == u"quality"_s && m.id == current && !m.tiers.contains(u"quality"_s)) {
            text = tr("Uses the fast model (install Quality for better rewrites)");
        }
        combo->addItem(text, m.id);
        if (m.id == current) {
            combo->setCurrentIndex(combo->count() - 1);
        }
    }
    if (combo->count() == 0) {
        combo->addItem(tr("No model installed"), QString());
    }
    combo->setEnabled(combo->count() > 1);
}

} // namespace paraphrase::app
