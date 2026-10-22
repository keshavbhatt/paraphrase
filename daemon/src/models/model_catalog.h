#pragma once

#include "util/result.h"

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

namespace paraphrase::models {

/// One file of a model, as `models.json` pins it.
struct ModelFile
{
    QString file; ///< a plain file name, never a path
    qint64 size = 0;
    QString sha256; ///< lowercase hex

    [[nodiscard]] bool operator==(const ModelFile&) const = default;
};

/// One catalog entry (DOCS/models.md, Catalog, schema 2). The daemon offers entries whose
/// role is `model` or `draft`; the others are for evaluation and are parsed only so the file
/// is checked whole.
struct ModelEntry
{
    QString id;
    QString name;
    QString role; ///< model | draft | candidate | experiment | judge
    QString family;
    QString hfRepo;   ///< "owner/name"
    QString revision; ///< 40 hex digits
    QList<ModelFile> files;
    QString licence;
    QString licenceUrl;
    int ctx = 0;
    QStringList tiers;          ///< fast, quality (role model only)
    double ramRequiredGb = 0.0; ///< total memory the computer needs; 0 when not set
    double memoryGb = 0.0;      ///< peak memory in use with its draft head; 0 when not set
    QStringList draftFor;       ///< role draft: the models it speeds up

    [[nodiscard]] bool isOffered() const;
    [[nodiscard]] qint64 downloadSize() const;
    /// Where a file comes from: Hugging Face at the pinned revision, over HTTPS.
    [[nodiscard]] QUrl url(const ModelFile& file) const;
};

/// The parsed catalog.
struct ModelCatalog
{
    static constexpr int kSchemaVersion = 2;

    int schemaVersion = 0;
    QList<ModelEntry> models;

    [[nodiscard]] const ModelEntry* find(const QString& id) const;
    /// The offered model for a tier, or nothing when the catalog has none.
    [[nodiscard]] const ModelEntry* modelForTier(const QString& tier) const;
    /// The draft head for `modelId`, or nothing.
    [[nodiscard]] const ModelEntry* draftFor(const QString& modelId) const;

    /// Strict, like Omnidict's catalog reader: a missing or malformed field is an error that
    /// names the entry. File names, repositories and checksums are checked because they
    /// become paths and URLs.
    [[nodiscard]] static util::Result<ModelCatalog> parse(const QByteArray& json);
    /// The catalog compiled into the daemon (ADR-015).
    [[nodiscard]] static ModelCatalog builtIn();
};

} // namespace paraphrase::models
