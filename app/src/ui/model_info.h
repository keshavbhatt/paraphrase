#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

namespace paraphrase::app {

/// One entry of the daemon's ListModels (DOCS/api.md), as the setup and settings windows use it.
struct ModelInfo
{
    QString id;
    QString name;
    QString role; ///< model | draft | candidate | ...
    QStringList tiers;
    QStringList draftFor;
    bool installed = false;
    bool fits = true;
    bool recommended = false;
    qint64 size = 0;
    qint64 onDisk = 0;
    double ramRequiredGb = 0;
    double memoryGb = 0;
    QString licence;
    QString licenceUrl;
    QString source;
    QString revision;
    QString sha256;
    QString path;
    /// The download, while one is queued, running, paused or failed.
    QString downloadState; ///< "" | waiting | downloading | verifying | paused | failed | installed
    qint64 received = 0;
    qint64 total = 0;
    QString downloadError;

    [[nodiscard]] bool isModel() const { return role == QStringLiteral("model"); }
    [[nodiscard]] bool isDraft() const { return role == QStringLiteral("draft"); }
    /// "huggingface.co/google/gemma-4-E2B-it-qat-q4_0-gguf": where the file comes from.
    [[nodiscard]] QString repository() const;
    /// "Google": the repository's owner, as the settings mock names the maker.
    [[nodiscard]] QString maker() const;
    /// Applies a DownloadProgress status for this model.
    void applyProgress(const QJsonObject& status);
};

/// The entries of a ListModels reply (a JSON array).
[[nodiscard]] QList<ModelInfo> parseModels(const QString& json);

/// Sizes as the mocks write them, in decimal units: "3.35 GB", "59 MB".
[[nodiscard]] QString formatSize(qint64 bytes);
/// "about 4.6 GB", "about 5 GB".
[[nodiscard]] QString formatMemory(double gb);

} // namespace paraphrase::app
