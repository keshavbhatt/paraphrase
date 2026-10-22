#include "model_catalog.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QtLogging>

#include <algorithm>
#include <array>

using namespace Qt::StringLiterals;

namespace paraphrase::models {
namespace {

constexpr std::array<QStringView, 5> kRoles = {u"model", u"draft", u"candidate", u"experiment", u"judge"};
constexpr std::array<QStringView, 2> kTiers = {u"fast", u"quality"};

template <size_t N>
bool oneOf(const std::array<QStringView, N>& values, const QString& value)
{
    return std::ranges::find(values, QStringView(value)) != values.end();
}

bool matches(const QString& pattern, const QString& value)
{
    return QRegularExpression(QRegularExpression::anchoredPattern(pattern)).match(value).hasMatch();
}

/// The files of an entry. A file name becomes a path under the models folder, so it can
/// name nothing outside it.
util::Result<QList<ModelFile>> parseFiles(const QJsonArray& files)
{
    using R = util::Result<QList<ModelFile>>;
    if (files.isEmpty()) {
        return R::failure(u"no files"_s);
    }
    QList<ModelFile> out;
    for (const auto& f : files) {
        const QJsonObject fo = f.toObject();
        const ModelFile file{
            .file = fo.value(u"file"_s).toString(),
            .size = fo.value(u"size"_s).toInteger(),
            .sha256 = fo.value(u"sha256"_s).toString(),
        };
        if (!matches(u"[A-Za-z0-9][A-Za-z0-9._-]*"_s, file.file) || file.file.contains(u".."_s)) {
            return R::failure(u"bad file name \"%1\""_s.arg(file.file));
        }
        if (file.size <= 0 || !matches(u"[0-9a-f]{64}"_s, file.sha256)) {
            return R::failure(u"%1: size and a lowercase sha256 are required"_s.arg(file.file));
        }
        out.append(file);
    }
    return R::success(out);
}

/// What only some roles carry: tiers and memory for a model, its targets for a draft.
util::Result<bool> parseRoleFields(const QJsonObject& o, ModelEntry& e)
{
    using R = util::Result<bool>;
    for (const auto& t : o.value(u"tiers"_s).toArray()) {
        if (!oneOf(kTiers, t.toString())) {
            return R::failure(u"unknown tier \"%1\""_s.arg(t.toString()));
        }
        e.tiers.append(t.toString());
    }
    for (const auto& d : o.value(u"draft_for"_s).toArray()) {
        e.draftFor.append(d.toString());
    }
    e.ramRequiredGb = o.value(u"ram_required"_s).toDouble();
    e.memoryGb = o.value(u"memory_gb"_s).toDouble();
    if (e.role == u"model"_s && (e.tiers.isEmpty() || e.ramRequiredGb <= 0.0 || e.memoryGb <= 0.0)) {
        return R::failure(u"a model needs tiers, ram_required and memory_gb"_s);
    }
    if (e.role == u"draft"_s && e.draftFor.isEmpty()) {
        return R::failure(u"a draft needs draft_for"_s);
    }
    return R::success(true);
}

/// Reads one entry; the error says what is wrong, the caller adds which entry.
util::Result<ModelEntry> parseEntry(const QJsonObject& o)
{
    using R = util::Result<ModelEntry>;
    const auto text = [&o](const QString& key) { return o.value(key).toString(); };
    ModelEntry e;
    e.id = text(u"id"_s);
    e.name = text(u"name"_s);
    e.role = text(u"role"_s);
    e.family = text(u"family"_s);
    e.hfRepo = text(u"hf_repo"_s);
    e.revision = text(u"revision"_s);
    e.licence = text(u"licence"_s);
    e.licenceUrl = text(u"licence_url"_s);
    e.ctx = o.value(u"ctx"_s).toInt();
    if (!matches(u"[a-z0-9][a-z0-9._-]*"_s, e.id)) {
        return R::failure(u"bad id"_s);
    }
    if (e.name.isEmpty() || e.licence.isEmpty() || !e.licenceUrl.startsWith(u"https://"_s) || e.ctx <= 0) {
        return R::failure(u"name, licence, licence_url (https) and ctx are required"_s);
    }
    if (!oneOf(kRoles, e.role)) {
        return R::failure(u"unknown role \"%1\""_s.arg(e.role));
    }
    if (!matches(u"[A-Za-z0-9][A-Za-z0-9._-]*/[A-Za-z0-9][A-Za-z0-9._-]*"_s, e.hfRepo)) {
        return R::failure(u"bad hf_repo"_s);
    }
    if (!matches(u"[0-9a-f]{40}"_s, e.revision)) {
        return R::failure(u"revision must be 40 hex digits"_s);
    }
    auto files = parseFiles(o.value(u"files"_s).toArray());
    if (!files.ok()) {
        return R::failure(files.error());
    }
    e.files = std::move(files).value();
    if (const auto roleFields = parseRoleFields(o, e); !roleFields.ok()) {
        return R::failure(roleFields.error());
    }
    return R::success(e);
}

} // namespace

bool ModelEntry::isOffered() const
{
    return role == u"model"_s || role == u"draft"_s;
}

qint64 ModelEntry::downloadSize() const
{
    qint64 total = 0;
    for (const auto& f : files) {
        total += f.size;
    }
    return total;
}

QUrl ModelEntry::url(const ModelFile& file) const
{
    return {u"https://huggingface.co/%1/resolve/%2/%3"_s.arg(hfRepo, revision, file.file)};
}

const ModelEntry* ModelCatalog::find(const QString& id) const
{
    for (const auto& m : models) {
        if (m.id == id) {
            return &m;
        }
    }
    return nullptr;
}

const ModelEntry* ModelCatalog::modelForTier(const QString& tier) const
{
    for (const auto& m : models) {
        if (m.role == u"model"_s && m.tiers.contains(tier)) {
            return &m;
        }
    }
    return nullptr;
}

const ModelEntry* ModelCatalog::draftFor(const QString& modelId) const
{
    for (const auto& m : models) {
        if (m.role == u"draft"_s && m.draftFor.contains(modelId)) {
            return &m;
        }
    }
    return nullptr;
}

util::Result<ModelCatalog> ModelCatalog::parse(const QByteArray& json)
{
    using R = util::Result<ModelCatalog>;
    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return R::failure(u"models.json is not a JSON object: %1"_s.arg(error.errorString()));
    }
    ModelCatalog catalog;
    catalog.schemaVersion = doc.object().value(u"schema_version"_s).toInt();
    if (catalog.schemaVersion != kSchemaVersion) {
        return R::failure(
            u"models.json schema %1, this daemon reads %2"_s.arg(catalog.schemaVersion).arg(kSchemaVersion));
    }
    const QJsonArray entries = doc.object().value(u"models"_s).toArray();
    for (qsizetype i = 0; i < entries.size(); ++i) {
        const QJsonObject o = entries.at(i).toObject();
        auto entry = parseEntry(o);
        if (!entry.ok()) {
            const QString id = o.value(u"id"_s).toString();
            return R::failure(
                u"models.json, %1: %2"_s.arg(id.isEmpty() ? u"entry %1"_s.arg(i) : id, entry.error()));
        }
        if (catalog.find(entry.value().id) != nullptr) {
            return R::failure(u"models.json: %1 is listed twice"_s.arg(entry.value().id));
        }
        catalog.models.append(std::move(entry).value());
    }
    for (const auto& m : catalog.models) {
        for (const auto& target : m.draftFor) {
            if (catalog.find(target) == nullptr) {
                return R::failure(u"models.json, %1: draft_for names unknown %2"_s.arg(m.id, target));
            }
        }
    }
    return R::success(catalog);
}

ModelCatalog ModelCatalog::builtIn()
{
    QFile file(u":/paraphrase/models.json"_s);
    if (!file.open(QIODevice::ReadOnly)) {
        qFatal("the built-in models.json is missing");
    }
    auto catalog = parse(file.readAll());
    if (!catalog.ok()) {
        // tst_models checks the shipped file, so this is a broken build.
        qFatal("%s", qPrintable(catalog.error()));
    }
    return std::move(catalog).value();
}

} // namespace paraphrase::models
