#pragma once

#include "model_catalog.h"
#include "model_store.h"

#include <QList>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QUrl>

#include <atomic>
#include <functional>
#include <memory>

class QFile;
class QNetworkAccessManager;
class QNetworkReply;

namespace paraphrase::models {

/// One model's place in the download queue.
struct DownloadStatus
{
    Q_GADGET

public:
    enum class State
    {
        Waiting,     ///< queued behind another model
        Downloading, ///< bytes arriving
        Verifying,   ///< checking a finished file against its sha256
        Paused,      ///< stopped by the user; the partial file is kept for a resume
        Failed,      ///< see `error`; Retry resumes where possible
        Installed,
    };
    Q_ENUM(State)

    QString modelId;
    State state = State::Waiting;
    qint64 received = 0; ///< bytes of the whole model on disk (finished files and the part)
    qint64 total = 0;    ///< the model's download size
    qint64 verified = 0; ///< bytes of the current file checked so far, while Verifying
    QString error;       ///< for the user, when Failed
};

/// Downloads models from the catalog (PLAN D8, 8; ADR-015), as Omnidict's DictionaryManager
/// does dictionaries: one model at a time, each file into `<file>.part`, resumed with an HTTP
/// Range request across restarts, then checked against its sha256 on the thread pool and
/// moved into place by ModelStore. Only HTTPS, never downgraded by a redirect. Lives on the
/// thread that owns the event loop.
class ModelDownloader : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ModelDownloader)

public:
    /// Where a file comes from; tests point it at a local server.
    using UrlResolver = std::function<QUrl(const ModelEntry&, const ModelFile&)>;

    explicit ModelDownloader(ModelStore store, QObject* parent = nullptr, UrlResolver resolver = {});
    ~ModelDownloader() override;

    [[nodiscard]] const ModelStore& store() const { return m_store; }
    [[nodiscard]] QList<DownloadStatus> downloads() const;

public Q_SLOTS:
    /// Queues `entry` (and resumes it when Paused or Failed). Installed files are skipped.
    void start(const paraphrase::models::ModelEntry& entry);
    /// Stops `modelId` and keeps what arrived, for a later start().
    void pause(const QString& modelId);
    /// Stops `modelId`, keeps the partial file, and forgets the download.
    void cancel(const QString& modelId);

Q_SIGNALS:
    void changed(const paraphrase::models::DownloadStatus& status);
    void installed(const QString& modelId);

private:
    struct Download;

    [[nodiscard]] Download* find(const QString& modelId) const;
    void startNext();
    void beginFile(Download& d);
    void onReadyRead(const QString& modelId);
    void onFinished(const QString& modelId);
    void onVerified(const QString& modelId, const util::Result<bool>& result);
    void fail(Download& d, const QString& message);
    void stop(Download& d);
    void emitChanged(const Download& d);
    [[nodiscard]] static bool writeAvailable(Download& d);
    [[nodiscard]] QUrl urlFor(const ModelEntry& entry, const ModelFile& file) const;

    ModelStore m_store;
    UrlResolver m_resolver;
    QNetworkAccessManager* m_network;             ///< child of this
    QList<std::shared_ptr<Download>> m_downloads; ///< queue order
};

} // namespace paraphrase::models

Q_DECLARE_METATYPE(paraphrase::models::DownloadStatus)
