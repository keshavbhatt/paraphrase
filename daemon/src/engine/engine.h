#pragma once

#include "generator.h"

#include <QString>

namespace paraphrase::engine {

struct EngineConfig
{
    QString modelPath;
    QString draftModelPath; ///< the model's MTP draft head (ADR-004), empty for none
    int threads = 0;        ///< 0: llama.cpp's default
    int gpuLayers = 0;      ///< layers offloaded to a GPU; 0 = CPU only
    int contextSize = 4096;
    int maxVariants = 1; ///< sequences decoded in one batch, at most
    bool flashAttention = true;
};

/// A model that runs requests: LlamaEngine in the daemon, a fake in the API tests. One
/// thread owns it (the inference thread); only cancel() may be called from another.
class Engine : public Generator
{
public:
    [[nodiscard]] virtual util::Result<bool> load(const EngineConfig& config) = 0;
    virtual void unload() = 0;
    [[nodiscard]] virtual bool isLoaded() const = 0;
    /// The config of the loaded model (its paths tell which one); empty when none is loaded.
    [[nodiscard]] virtual EngineConfig loadedConfig() const = 0;
    /// Tokens in `text` as plain text: for the skills' input limits.
    [[nodiscard]] virtual int countTokens(const QString& text) const = 0;
    /// Reads the request's prompt into the cache without generating (ADR-005): if the same
    /// request follows, only its last token is read again.
    [[nodiscard]] virtual util::Result<bool> warm(const GenerationRequest& request) = 0;
    /// Stops the current generation at the next token; its finish reason is "cancelled". The
    /// flag stays set, so every later generate() stops at once, until resetCancel(): a cancel
    /// that arrives between a job's model calls (the language retry) still counts.
    virtual void cancel() = 0;
    virtual void resetCancel() = 0;
};

} // namespace paraphrase::engine
