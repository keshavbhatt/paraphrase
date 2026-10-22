#pragma once

#include "model_catalog.h"
#include "util/result.h"

#include <QString>

#include <atomic>
#include <functional>

namespace paraphrase::models {

/// The models folder on disk: `<root>/<model id>/<file>`, the layout scripts/fetch_model.py
/// writes, so models fetched for development count as installed. A file is installed when
/// it was checked against its sha256 and has not changed since: the check leaves a stamp
/// `<file>.sha256` holding "<sha256> <size> <mtime in ns>", the same as fetch_model.py's.
/// Checking a 3 GB file takes seconds, so it runs once, after the download.
class ModelStore
{
public:
    explicit ModelStore(QString root);

    /// ADR-009: `$SNAP_USER_COMMON/models` in a snap, else `<data folder>/paraphrase/models`
    /// (`$XDG_DATA_HOME`, or `~/.var/app/<id>/data` in a Flatpak). PARAPHRASE_MODELS_DIR
    /// overrides it (development: the repository's models/).
    [[nodiscard]] static QString defaultRoot();

    [[nodiscard]] const QString& root() const { return m_root; }
    [[nodiscard]] QString path(const ModelEntry& entry, const ModelFile& file) const;
    /// Where a download collects before it is checked: `<file>.part`.
    [[nodiscard]] QString partPath(const ModelEntry& entry, const ModelFile& file) const;
    /// The file llama.cpp loads (split models find their other parts).
    [[nodiscard]] QString modelPath(const ModelEntry& entry) const;

    [[nodiscard]] bool isInstalled(const ModelEntry& entry) const;
    [[nodiscard]] bool isVerified(const ModelEntry& entry, const ModelFile& file) const;
    /// Bytes on disk for `entry`, finished files and partial downloads.
    [[nodiscard]] qint64 usedBytes(const ModelEntry& entry) const;

    /// Checks `<file>.part` against the catalog (size, then sha256) and moves it into place
    /// with a stamp. A wrong file is deleted. `progress` gets the bytes hashed so far;
    /// `cancel` stops the check and keeps the part file.
    [[nodiscard]] util::Result<bool> installPart(const ModelEntry& entry, const ModelFile& file,
                                                 const std::function<void(qint64)>& progress = {},
                                                 const std::atomic<bool>* cancel = nullptr) const;
    /// Removes the model's folder, downloads in progress included.
    [[nodiscard]] util::Result<bool> remove(const ModelEntry& entry) const;

    /// Streams `path` through sha256; empty when it cannot be read or was cancelled.
    [[nodiscard]] static QString sha256Of(const QString& path,
                                          const std::function<void(qint64)>& progress = {},
                                          const std::atomic<bool>* cancel = nullptr);

private:
    [[nodiscard]] static QString stampPath(const QString& path);
    [[nodiscard]] static QString stampText(const QString& path, const ModelFile& file);

    QString m_root;
};

} // namespace paraphrase::models
