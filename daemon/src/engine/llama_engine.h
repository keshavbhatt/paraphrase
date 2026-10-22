#pragma once

#include "engine.h"

#include <QString>

#include <atomic>
#include <memory>
#include <vector>

struct common_init_result;
struct common_params;
struct common_params_sampling;
struct common_chat_templates;
struct common_speculative;
struct common_speculative_init_result;

namespace paraphrase::engine {

/// The only code that talks to llama.cpp (DOCS/CODING_STANDARDS.md 10). It loads the model and
/// builds prompts the way llama-server does (common_init_from_params, the model's own chat
/// template through llama.cpp's chat session, thinking off per PLAN D12, the model file's
/// sampling defaults under the skill's), so the daemon's prompts and answers match the M0
/// measurements. Two deliberate differences from llama-server: the user's text is tokenised
/// as plain text, never parsed for control tokens (captured text is untrusted), and the
/// sliding-window attention cache is kept full size so any cached prompt prefix can be reused.
///
/// Not thread-safe: one thread owns it (the inference thread); cancel() may be called from
/// any thread.
class LlamaEngine final : public Engine
{
public:
    LlamaEngine();
    ~LlamaEngine() override; // out of line: the llama.cpp types are incomplete here
    LlamaEngine(const LlamaEngine&) = delete;
    LlamaEngine& operator=(const LlamaEngine&) = delete;
    LlamaEngine(LlamaEngine&&) = delete;
    LlamaEngine& operator=(LlamaEngine&&) = delete;

    [[nodiscard]] util::Result<bool> load(const EngineConfig& config) override;
    void unload() override;
    [[nodiscard]] bool isLoaded() const override;
    [[nodiscard]] EngineConfig loadedConfig() const override;
    [[nodiscard]] int countTokens(const QString& text) const override;
    [[nodiscard]] util::Result<bool> warm(const GenerationRequest& request) override;
    [[nodiscard]] double loadSeconds() const { return m_loadSeconds; }
    /// Whether requests with `draft` set are drafted: a draft head is loaded and usable.
    [[nodiscard]] bool canDraft() const { return m_spec != nullptr; }

    [[nodiscard]] util::Result<Generation> generate(const GenerationRequest& request,
                                                    const TextSink& sink) override;

    /// The prompt's tokens as generate() feeds them to the model (empty when no model is
    /// loaded): for input limits, and for tests that hold the prompt to llama-server's.
    [[nodiscard]] std::vector<int> promptTokens(const GenerationRequest& request) const;

    void cancel() override;
    void resetCancel() override;

private:
    struct TemplatesDeleter
    {
        void operator()(common_chat_templates* tmpls) const;
    };
    struct SpeculativeDeleter
    {
        void operator()(common_speculative* spec) const;
    };

    void loadDraft(const EngineConfig& config, common_params& params);
    class Run; // one generate() call, in llama_engine.cpp

    std::unique_ptr<common_init_result> m_init;         ///< owns the model and the context
    std::unique_ptr<common_params_sampling> m_defaults; ///< after the model file's defaults
    std::unique_ptr<common_chat_templates, TemplatesDeleter> m_templates;
    // Declared after m_init and freed before it: the draft context reads the target's.
    std::unique_ptr<common_speculative_init_result> m_draftInit; ///< the draft head and its context
    std::unique_ptr<common_speculative, SpeculativeDeleter> m_spec;
    EngineConfig m_config;
    std::vector<int> m_cachedTokens;      ///< the tokens of sequence 0 in the KV cache
    std::vector<int> m_draftCachedTokens; ///< the same for the draft context
    std::atomic<bool> m_cancel{false};
    double m_loadSeconds = 0.0;
};

} // namespace paraphrase::engine
