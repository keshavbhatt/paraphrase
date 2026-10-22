#include "tier_selector.h"

using namespace Qt::StringLiterals;

namespace paraphrase::models {

TierSelector::TierSelector(const ModelCatalog& catalog, HardwareInfo hardware)
    : m_catalog(catalog)
    , m_hardware(std::move(hardware))
{}

bool TierSelector::fits(const ModelEntry& model) const
{
    return m_hardware.totalMemoryGb() >= model.ramRequiredGb * kMemoryTolerance;
}

QList<ModelOffer> TierSelector::offers() const
{
    QList<ModelOffer> out;
    for (const QString& tier : {u"fast"_s, u"quality"_s}) {
        if (const ModelEntry* m = m_catalog.modelForTier(tier)) {
            out.append({.model = m, .fits = fits(*m), .recommended = tier == u"fast"_s});
        }
    }
    return out;
}

const ModelEntry* TierSelector::modelFor(const QString& skillTier, const QSet<QString>& installed) const
{
    const auto usable = [&](const QString& tier) -> const ModelEntry* {
        const ModelEntry* m = m_catalog.modelForTier(tier);
        return m != nullptr && installed.contains(m->id) && fits(*m) ? m : nullptr;
    };
    const auto ready = [&](const QString& tier) -> const ModelEntry* {
        const ModelEntry* m = m_catalog.modelForTier(tier);
        return m != nullptr && installed.contains(m->id) ? m : nullptr;
    };
    if (skillTier == u"quality"_s) {
        if (const ModelEntry* m = usable(u"quality"_s)) {
            return m;
        }
    }
    // The fast model runs everywhere it is installed, even below its memory guide (ADR-003);
    // the quality model stands in only when it is the one installed and fits.
    if (const ModelEntry* m = ready(u"fast"_s)) {
        return m;
    }
    return usable(u"quality"_s);
}

} // namespace paraphrase::models
