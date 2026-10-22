#pragma once

#include "hardware_probe.h"
#include "model_catalog.h"

#include <QList>
#include <QSet>
#include <QString>

namespace paraphrase::models {

/// A model the first-run and settings screens offer, and whether this computer can run it.
struct ModelOffer
{
    const ModelEntry* model = nullptr;
    bool fits = false;        ///< enough memory (see TierSelector::fits)
    bool recommended = false; ///< the fast tier: the one model everyone needs
};

/// Picks models for this computer (PLAN D9, 5.3; ADR-016). Two tiers: fast (proofread, reply)
/// and quality (rewrite and longer writing). The quality model is offered only where it fits;
/// a skill that asks for it runs on the fast model when it is not installed or does not fit.
class TierSelector
{
public:
    /// "8 GB" computers report about 7.6 GB, "16 GB" ones about 15.3 GB.
    static constexpr double kMemoryTolerance = 0.9;

    TierSelector(const ModelCatalog& catalog, HardwareInfo hardware);

    [[nodiscard]] bool fits(const ModelEntry& model) const;
    /// Every offered model, the fast tier first.
    [[nodiscard]] QList<ModelOffer> offers() const;
    /// The model for a skill's tier (fast, quality, any) among the installed ones; nothing
    /// when none is installed.
    [[nodiscard]] const ModelEntry* modelFor(const QString& skillTier, const QSet<QString>& installed) const;

private:
    const ModelCatalog& m_catalog;
    HardwareInfo m_hardware;
};

} // namespace paraphrase::models
