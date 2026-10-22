#include "skill_registry.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QRegularExpression>

#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <yaml-cpp/yaml.h>

using namespace Qt::StringLiterals;

namespace paraphrase::skills {
namespace {

constexpr std::array<QStringView, 10> kRequired = {
    u"id",       u"version", u"title",  u"tier",     u"max_input_tokens", u"max_output_tokens",
    u"sampling", u"output",  u"system", u"template",
};
constexpr std::array<QStringView, 3> kTiers = {u"fast", u"quality", u"any"};
constexpr std::array<QStringView, 2> kOutputTypes = {u"text", u"json"};
constexpr std::array<QStringView, 2> kOutputLanguage = {u"same", u"any"};
constexpr std::array<QStringView, 2> kSpeculative = {u"none", u"draft"};
constexpr std::array<QStringView, 6> kSamplingKeys = {
    u"temperature", u"top_p", u"top_k", u"min_p", u"repeat_penalty", u"seed",
};
/// Values the engine provides (PLAN 5.1, DOCS/skills.md); presets add their own keys.
constexpr std::array<QStringView, 8> kKnownValues = {
    u"text", u"context", u"lang", u"lang_name", u"tone", u"intent", u"target_lang", u"target_lang_name",
};

bool oneOf(std::span<const QStringView> allowed, QStringView value)
{
    return std::ranges::find(allowed, value) != allowed.end();
}

QString fromStd(const std::string& s)
{
    return QString::fromStdString(s);
}

/// A plain scalar as PyYAML (YAML 1.1) reads it, so both loaders agree (ADR-014).
QJsonValue plainScalar(const QString& s)
{
    static const QRegularExpression nullRe(u"^(?:~|null|Null|NULL|)$"_s);
    static const QRegularExpression trueRe(u"^(?:yes|Yes|YES|true|True|TRUE|on|On|ON)$"_s);
    static const QRegularExpression falseRe(u"^(?:no|No|NO|false|False|FALSE|off|Off|OFF)$"_s);
    static const QRegularExpression intRe(u"^[-+]?(?:0|[1-9][0-9_]*)$"_s);
    static const QRegularExpression floatRe(
        u"^(?:[-+]?(?:[0-9][0-9_]*)\\.[0-9_]*(?:[eE][-+][0-9]+)?|\\.[0-9][0-9_]*(?:[eE][-+][0-9]+)?)$"_s);
    if (nullRe.match(s).hasMatch()) {
        return QJsonValue::Null;
    }
    if (trueRe.match(s).hasMatch()) {
        return true;
    }
    if (falseRe.match(s).hasMatch()) {
        return false;
    }
    QString digits = s;
    digits.remove(u'_');
    if (intRe.match(s).hasMatch()) {
        return {digits.toLongLong()};
    }
    if (floatRe.match(s).hasMatch()) {
        return {digits.toDouble()};
    }
    return s;
}

QJsonValue toJson(const YAML::Node& node)
{
    switch (node.Type()) {
    case YAML::NodeType::Scalar: {
        const QString text = fromStd(node.Scalar());
        return node.Tag() == "!" ? QJsonValue(text) : plainScalar(text); // "!" = quoted
    }
    case YAML::NodeType::Sequence: {
        QJsonArray array;
        for (const auto& child : node) {
            array.append(toJson(child));
        }
        return array;
    }
    case YAML::NodeType::Map: {
        QJsonObject object;
        for (const auto& pair : node) {
            object.insert(fromStd(pair.first.as<std::string>()), toJson(pair.second));
        }
        return object;
    }
    case YAML::NodeType::Null:
    case YAML::NodeType::Undefined:
        return QJsonValue::Null;
    }
    return QJsonValue::Null;
}

QString checkFields(const QJsonObject& raw, const QString& name)
{
    QStringList missing;
    for (const QStringView key : kRequired) {
        if (!raw.contains(key)) {
            missing.append(key.toString());
        }
    }
    if (!missing.isEmpty()) {
        return u"%1: missing %2"_s.arg(name, missing.join(u", "_s));
    }
    if (const int version = raw.value(u"version"_s).toInt();
        version < kMinSchemaVersion || version > kSchemaVersion) {
        return u"%1: version %2 is not schema %3 to %4"_s.arg(name)
            .arg(version)
            .arg(kMinSchemaVersion)
            .arg(kSchemaVersion);
    }
    if (raw.contains(u"user_presets"_s)) {
        const QJsonArray values = raw.value(u"user_presets"_s).toObject().value(u"values"_s).toArray();
        if (values.isEmpty() ||
            std::ranges::any_of(values, [](const QJsonValue& v) { return !v.isString(); })) {
            return u"%1: user_presets.values must list the preset values the user fills"_s.arg(name);
        }
    }
    if (!oneOf(kTiers, raw.value(u"tier"_s).toString())) {
        return u"%1: tier must be one of fast, quality, any"_s.arg(name);
    }
    QStringList unknown;
    const QJsonObject sampling = raw.value(u"sampling"_s).toObject();
    for (auto it = sampling.begin(); it != sampling.end(); ++it) {
        if (!oneOf(kSamplingKeys, it.key())) {
            unknown.append(it.key());
        }
    }
    if (!unknown.isEmpty()) {
        unknown.sort();
        return u"%1: unknown sampling keys %2"_s.arg(name, unknown.join(u", "_s));
    }
    if (!oneOf(kSpeculative, raw.value(u"speculative"_s).toString(u"none"_s))) {
        return u"%1: speculative must be one of none, draft"_s.arg(name);
    }
    if (raw.value(u"system"_s).toString().contains(u"{{"_s)) {
        return u"%1: system must be constant so its KV cache is shared"_s.arg(name);
    }
    return {};
}

QString checkOutput(const QJsonObject& raw, const QString& name)
{
    const QJsonObject output = raw.value(u"output"_s).toObject();
    const QString type = output.value(u"type"_s).toString();
    if (!oneOf(kOutputTypes, type)) {
        return u"%1: output.type must be one of text, json"_s.arg(name);
    }
    if (!oneOf(kOutputLanguage, output.value(u"language"_s).toString(u"any"_s))) {
        return u"%1: output.language must be one of same, any"_s.arg(name);
    }
    if (raw.value(u"variants"_s).toInt(1) > 1 && type != u"text"_s) {
        return u"%1: with variants > 1 each variant is one text output"_s.arg(name);
    }
    if (type == u"json"_s && !raw.contains(u"grammar"_s)) {
        return u"%1: json output needs a grammar (PLAN D5)"_s.arg(name);
    }
    if (const QJsonValue min = output.value(u"min_length"_s); !min.isUndefined()) {
        if (!min.isDouble() || min.toDouble() <= 0.0 || min.toDouble() >= 1.0) {
            return u"%1: output.min_length must be a number between 0 and 1"_s.arg(name);
        }
        if (type != u"text"_s || raw.value(u"variants"_s).toInt(1) > 1) {
            return u"%1: output.min_length needs one text output"_s.arg(name);
        }
    }
    return {};
}

QString checkTemplate(const Skill& skill)
{
    QStringList unknown;
    for (const QString& var : templateVariables(skill.templ)) {
        if (!oneOf(kKnownValues, var) && !skill.preset.contains(var) && !skill.userValues.contains(var)) {
            unknown.append(var);
        }
    }
    unknown.sort();
    return unknown.isEmpty() ? QString()
                             : u"%1: unknown template values %2"_s.arg(skill.id, unknown.join(u", "_s));
}

/// The grammar text ("" when the skill has none). It must live in the skill file's folder.
util::Result<QString> readGrammar(const QFileInfo& info, const QJsonObject& raw, const QString& name)
{
    using R = util::Result<QString>;
    const QJsonValue path = raw.value(u"grammar"_s);
    if (path.isUndefined() || path.isNull()) {
        return R::success({});
    }
    const QDir dir = info.absoluteDir();
    const QFileInfo grammar(dir.filePath(path.toString()));
    if (!grammar.exists() || !grammar.canonicalFilePath().startsWith(dir.canonicalPath() + u'/')) {
        return R::failure(u"%1: grammar must live under skills/"_s.arg(name));
    }
    QFile file(grammar.canonicalFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return R::failure(u"%1: cannot read the grammar"_s.arg(name));
    }
    return R::success(QString::fromUtf8(file.readAll()));
}

Skill baseSkill(const QJsonObject& raw, QString grammar)
{
    Skill s;
    s.baseId = raw.value(u"id"_s).toString();
    s.version = raw.value(u"version"_s).toInt();
    s.revision = raw.value(u"revision"_s).toInt(1);
    s.tier = raw.value(u"tier"_s).toString();
    s.maxInputTokens = raw.value(u"max_input_tokens"_s).toInt();
    s.maxOutputTokens = raw.value(u"max_output_tokens"_s).toInt();
    s.variants = raw.value(u"variants"_s).toInt(1);
    s.speculative = raw.value(u"speculative"_s).toString(u"none"_s);
    s.sampling = raw.value(u"sampling"_s).toObject();
    s.output = raw.value(u"output"_s).toObject();
    s.system = raw.value(u"system"_s).toString();
    s.templ = raw.value(u"template"_s).toString();
    s.grammar = std::move(grammar);
    for (const auto& v : raw.value(u"user_presets"_s).toObject().value(u"values"_s).toArray()) {
        s.userValues.append(v.toString());
    }
    return s;
}

/// One skill per preset, in the file's order (QJsonObject would sort them, so this walks
/// the YAML tree), or the base skill alone.
QList<Skill> expandPresets(const YAML::Node& doc, Skill base, const QString& title)
{
    // Index through a const node: on a missing key yaml-cpp then returns an undefined node
    // instead of an invalid one that throws when touched.
    const YAML::Node& constDoc = doc;
    const YAML::Node presets = constDoc["presets"];
    if (!presets || !presets.IsMap() || presets.size() == 0) {
        base.id = base.baseId;
        base.title = title;
        return {base};
    }
    QList<Skill> out;
    for (const auto& pair : presets) {
        const QString key = fromStd(pair.first.as<std::string>());
        const QJsonObject preset = toJson(pair.second).toObject();
        Skill s = base;
        s.id = base.baseId + u'.' + key;
        s.title = title + u": "_s + preset.value(u"title"_s).toString(key);
        for (auto it = preset.begin(); it != preset.end(); ++it) {
            if (it.key() != u"title"_s) {
                s.preset.insert(it.key(), it.value().isString() ? it.value().toString()
                                                                : it.value().toVariant().toString());
            }
        }
        out.append(s);
    }
    return out;
}

} // namespace

util::Result<QList<Skill>> SkillRegistry::loadFile(const QString& path)
{
    using R = util::Result<QList<Skill>>;
    const QFileInfo info(path);
    const QString name = info.fileName();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return R::failure(u"%1: cannot read the file"_s.arg(name));
    }
    YAML::Node doc;
    QJsonObject raw;
    try {
        doc = YAML::Load(file.readAll().toStdString());
        raw = toJson(doc).toObject();
    } catch (const YAML::Exception& e) {
        return R::failure(u"%1: not valid YAML: %2"_s.arg(name, fromStd(e.what())));
    }
    for (const QString& error : {checkFields(raw, name), checkOutput(raw, name)}) {
        if (!error.isEmpty()) {
            return R::failure(error);
        }
    }
    const auto grammar = readGrammar(info, raw, name);
    if (!grammar.ok()) {
        return R::failure(grammar.error());
    }
    const QList<Skill> out =
        expandPresets(doc, baseSkill(raw, grammar.value()), raw.value(u"title"_s).toString());
    for (const Skill& s : out) {
        if (const QString error = checkTemplate(s); !error.isEmpty()) {
            return R::failure(error);
        }
    }
    return R::success(out);
}

util::Result<SkillRegistry> SkillRegistry::load(const QString& skillsDir, const QStringList& overrides)
{
    using R = util::Result<SkillRegistry>;
    SkillRegistry registry;
    const QDir dir(skillsDir);
    if (!dir.exists()) {
        return R::failure(u"skills folder not found: %1"_s.arg(skillsDir));
    }
    for (const QString& entry : dir.entryList({u"*.yaml"_s}, QDir::Files, QDir::Name)) {
        const auto loaded = loadFile(dir.filePath(entry));
        if (!loaded.ok()) {
            return R::failure(loaded.error());
        }
        for (const Skill& s : loaded.value()) {
            if (registry.find(s.id) != nullptr) {
                return R::failure(u"duplicate skill id %1"_s.arg(s.id));
            }
            if (s.takesUserPresets() && s.preset.isEmpty()) {
                registry.m_userBases.append(s); // a template for the user's actions
                continue;
            }
            registry.m_skills.append(s);
        }
    }
    for (const QString& path : overrides) {
        const auto loaded = loadFile(path);
        if (!loaded.ok()) {
            return R::failure(loaded.error());
        }
        for (const Skill& s : loaded.value()) {
            const auto it =
                std::ranges::find_if(registry.m_skills, [&s](const Skill& x) { return x.id == s.id; });
            if (it != registry.m_skills.end()) {
                *it = s;
            } else {
                registry.m_skills.append(s);
            }
        }
    }
    registry.m_builtIn = registry.m_skills.size();
    return R::success(registry);
}

void SkillRegistry::setUserActions(const QList<UserAction>& actions)
{
    m_skills.resize(m_builtIn);
    if (m_userBases.isEmpty()) {
        return;
    }
    const Skill& base = m_userBases.front();
    for (const UserAction& a : actions) {
        Skill s = base;
        s.id = base.baseId + u'.' + a.id;
        s.title = base.title + u": "_s + a.name;
        s.tier = a.tier;
        s.preset.insert(base.userValues.front(), a.instruction);
        m_skills.append(s);
    }
}

const Skill* SkillRegistry::find(const QString& id) const
{
    const auto it = std::ranges::find_if(m_skills, [&id](const Skill& s) { return s.id == id; });
    return it == m_skills.end() ? nullptr : &*it;
}

} // namespace paraphrase::skills
