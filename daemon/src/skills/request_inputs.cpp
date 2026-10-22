#include "request_inputs.h"

using namespace Qt::StringLiterals;

namespace paraphrase::skills {

Values buildInputs(const QString& baseSkillId, const RequestFields& fields,
                   const util::LanguageDetector& detector)
{
    Values inputs;
    const auto addLanguage = [&inputs, &detector](const QString& source) {
        const QString lang = detector.detect(source);
        inputs.insert(u"lang"_s, lang);
        inputs.insert(u"lang_name"_s, detector.promptName(lang));
    };
    if (baseSkillId == u"reply"_s) {
        inputs.insert(u"context"_s, fields.context);
        inputs.insert(u"text"_s, fields.text);
        addLanguage(fields.context);
        return inputs;
    }
    inputs.insert(u"text"_s, fields.text);
    addLanguage(fields.text);
    if (!fields.targetLang.isEmpty()) {
        // The default prompt name tells the model to keep the input's language, which would
        // contradict the user's choice: use plain names instead.
        const QString plain = detector.plainName(inputs.value(u"lang"_s));
        inputs.insert(u"lang_name"_s, plain.isEmpty() ? u"its own language"_s : plain);
        inputs.insert(u"target_lang"_s, fields.targetLang);
        inputs.insert(u"target_lang_name"_s, detector.plainName(fields.targetLang));
    }
    return inputs;
}

} // namespace paraphrase::skills
