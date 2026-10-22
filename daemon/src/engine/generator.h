#pragma once

#include "util/result.h"

#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <functional>

namespace paraphrase::engine {

/// One model call: the rendered messages, the skill's sampling, grammar and limits.
struct GenerationRequest
{
    QString system;
    QString user;
    QJsonObject sampling; ///< temperature, top_p, top_k, min_p, repeat_penalty, seed (skill schema)
    QString grammar;      ///< GBNF, empty for none
    int maxTokens = 256;
    int variants = 1; ///< completions decoded in one batch from one prompt
};

/// What came back, with the timings the latency bench reports (PLAN 5.4).
struct Generation
{
    QStringList texts; ///< one per variant
    QString finishReason;
    double ttftSeconds = 0.0;  ///< to the first streamed text
    double totalSeconds = 0.0; ///< the whole call
    int promptTokens = 0;      ///< tokens in the prompt
    int cachedTokens = 0;      ///< of those, reused from the previous request
    int generatedTokens = 0;   ///< all variants
};

/// Called with each new piece of text as it is generated (complete UTF-8 only).
using TextSink = std::function<void(int variant, const QString& piece)>;

/// Something that runs a GenerationRequest: the llama.cpp engine, or a test double.
class Generator
{
public:
    Generator() = default;
    virtual ~Generator() = default;
    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;
    Generator(Generator&&) = delete;
    Generator& operator=(Generator&&) = delete;

    [[nodiscard]] virtual util::Result<Generation> generate(const GenerationRequest& request,
                                                            const TextSink& sink) = 0;
};

} // namespace paraphrase::engine
