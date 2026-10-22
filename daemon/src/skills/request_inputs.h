#pragma once

#include "template_renderer.h"
#include "util/language_detector.h"

#include <QString>

namespace paraphrase::skills {

/// What a client sends with a request (PLAN 5.1): the text, the message it answers, and
/// the output language when the user chose one.
struct RequestFields
{
    QString text;
    QString context;
    QString targetLang;
    QString intent; ///< what the user wants to say, in their words (a full reply)
    QString tone;   ///< a tone the user picked, for a skill whose prompt takes one
};

/// The template values the engine adds to a request: `lang` and `lang_name` from the
/// detector (run on `context` for a skill that answers a message, its `mainInput` being
/// "context", on `text` otherwise), and with a user-chosen
/// `target_lang` the plain names of both languages (DOCS/skills.md). A port of
/// eval/paraeval/common.skill_inputs.
[[nodiscard]] Values buildInputs(const QString& mainInput, const RequestFields& fields,
                                 const util::LanguageDetector& detector);

} // namespace paraphrase::skills
