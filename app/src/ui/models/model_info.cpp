#include "ui/models/model_info.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr double kGB = 1e9;
constexpr double kMB = 1e6;

QStringList strings(const QJsonValue& v)
{
    QStringList out;
    for (const auto& s : v.toArray()) {
        out.append(s.toString());
    }
    return out;
}

} // namespace

QString ModelInfo::repository() const
{
    // .../google/gemma-4-E2B-it-qat-q4_0-gguf/resolve/<revision>/<file>
    const QUrl url(source);
    const QStringList parts = url.path().split(u'/', Qt::SkipEmptyParts);
    if (parts.size() < 2) {
        return url.host();
    }
    return url.host() + u'/' + parts.at(0) + u'/' + parts.at(1);
}

QString ModelInfo::maker() const
{
    const QStringList parts = QUrl(source).path().split(u'/', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return {};
    }
    QString owner = parts.front();
    if (!owner.isEmpty()) {
        owner[0] = owner.at(0).toUpper();
    }
    return owner;
}

void ModelInfo::applyProgress(const QJsonObject& status)
{
    downloadState = status.value(u"state"_s).toString();
    received = status.value(u"received"_s).toInteger();
    total = status.value(u"total"_s).toInteger();
    downloadError = status.value(u"error"_s).toString();
    if (downloadState == u"installed"_s) {
        installed = true;
    }
}

QList<ModelInfo> parseModels(const QString& json)
{
    QList<ModelInfo> out;
    for (const auto& v : QJsonDocument::fromJson(json.toUtf8()).array()) {
        const QJsonObject o = v.toObject();
        ModelInfo m{
            .id = o.value(u"id"_s).toString(),
            .name = o.value(u"name"_s).toString(),
            .role = o.value(u"role"_s).toString(),
            .tiers = strings(o.value(u"tiers"_s)),
            .draftFor = strings(o.value(u"draft_for"_s)),
            .installed = o.value(u"installed"_s).toBool(),
            .fits = o.value(u"fits"_s).toBool(true),
            .recommended = o.value(u"recommended"_s).toBool(),
            .size = o.value(u"size"_s).toInteger(),
            .onDisk = o.value(u"on_disk"_s).toInteger(),
            .ramRequiredGb = o.value(u"ram_required_gb"_s).toDouble(),
            .memoryGb = o.value(u"memory_gb"_s).toDouble(),
            .licence = o.value(u"licence"_s).toString(),
            .licenceUrl = o.value(u"licence_url"_s).toString(),
            .source = o.value(u"source"_s).toString(),
            .revision = o.value(u"revision"_s).toString(),
            .sha256 = o.value(u"sha256"_s).toString(),
            .path = o.value(u"path"_s).toString(),
            .downloadState = {},
            .received = 0,
            .total = 0,
            .downloadError = {},
        };
        if (const QJsonObject d = o.value(u"download"_s).toObject(); !d.isEmpty()) {
            m.applyProgress(d);
        }
        out.append(m);
    }
    return out;
}

QString formatSize(qint64 bytes)
{
    const auto b = static_cast<double>(bytes);
    if (b >= kGB) {
        return QCoreApplication::translate("Models", "%1 GB").arg(b / kGB, 0, 'f', 2);
    }
    return QCoreApplication::translate("Models", "%1 MB").arg(qRound(b / kMB));
}

QString formatMemory(double gb)
{
    const bool whole = qFuzzyCompare(gb, static_cast<double>(qRound(gb)));
    return QCoreApplication::translate("Models", "about %1 GB").arg(gb, 0, 'f', whole ? 0 : 1);
}

} // namespace paraphrase::app
