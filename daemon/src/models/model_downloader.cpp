#include "model_downloader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QStorageInfo>
#include <QThreadPool>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcDownloads, "paraphrase.downloads")

namespace paraphrase::models {
namespace {

constexpr int kStallTimeoutMs = 60'000;              ///< a connection silent this long is dropped
constexpr qint64 kSpaceMargin = 256LL * 1024 * 1024; ///< kept free after the download

} // namespace

struct ModelDownloader::Download
{
    ModelEntry entry;
    DownloadStatus status;
    qsizetype fileIndex = 0; ///< the file in progress
    QNetworkReply* reply = nullptr;
    std::unique_ptr<QFile> part;
    qint64 resumeOffset = 0; ///< bytes on disk when this request began
    bool headerChecked = false;
    std::shared_ptr<std::atomic<bool>> cancelVerify = std::make_shared<std::atomic<bool>>(false);

    [[nodiscard]] const ModelFile& file() const { return entry.files.at(fileIndex); }
};

ModelDownloader::ModelDownloader(ModelStore store, QObject* parent, UrlResolver resolver)
    : QObject(parent)
    , m_store(std::move(store))
    , m_resolver(std::move(resolver))
    , m_network(new QNetworkAccessManager(this))
{
    m_network->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    m_network->setTransferTimeout(kStallTimeoutMs);
}

ModelDownloader::~ModelDownloader()
{
    for (const auto& d : m_downloads) {
        stop(*d);
    }
}

QList<DownloadStatus> ModelDownloader::downloads() const
{
    QList<DownloadStatus> out;
    for (const auto& d : m_downloads) {
        out.append(d->status);
    }
    return out;
}

ModelDownloader::Download* ModelDownloader::find(const QString& modelId) const
{
    for (const auto& d : m_downloads) {
        if (d->entry.id == modelId) {
            return d.get();
        }
    }
    return nullptr;
}

QUrl ModelDownloader::urlFor(const ModelEntry& entry, const ModelFile& file) const
{
    return m_resolver ? m_resolver(entry, file) : entry.url(file);
}

void ModelDownloader::emitChanged(const Download& d)
{
    Q_EMIT changed(d.status);
}

void ModelDownloader::start(const ModelEntry& entry)
{
    Download* d = find(entry.id);
    if (d == nullptr) {
        const auto created = std::make_shared<Download>();
        created->entry = entry;
        created->status.modelId = entry.id;
        created->status.total = entry.downloadSize();
        m_downloads.append(created);
        d = created.get();
    } else if (d->status.state != DownloadStatus::State::Paused &&
               d->status.state != DownloadStatus::State::Failed) {
        return; // already queued or running
    }
    d->status.state = DownloadStatus::State::Waiting;
    d->status.error.clear();
    d->status.received = m_store.usedBytes(entry);
    emitChanged(*d);
    startNext();
}

void ModelDownloader::startNext()
{
    for (const auto& d : m_downloads) {
        const auto state = d->status.state;
        if (state == DownloadStatus::State::Downloading || state == DownloadStatus::State::Verifying) {
            return; // one at a time: models are gigabytes
        }
    }
    for (const auto& d : m_downloads) {
        if (d->status.state == DownloadStatus::State::Waiting) {
            d->fileIndex = 0;
            beginFile(*d);
            return;
        }
    }
}

void ModelDownloader::beginFile(Download& d)
{
    while (d.fileIndex < d.entry.files.size() && m_store.isVerified(d.entry, d.file())) {
        ++d.fileIndex; // already installed
    }
    if (d.fileIndex == d.entry.files.size()) {
        d.status.state = DownloadStatus::State::Installed;
        d.status.received = d.status.total;
        qCInfo(lcDownloads) << "installed" << d.entry.id;
        emitChanged(d);
        const QString id = d.entry.id;
        m_downloads.removeIf([&](const auto& x) { return x->entry.id == id; });
        Q_EMIT installed(id);
        startNext();
        return;
    }

    const QString partPath = m_store.partPath(d.entry, d.file());
    QDir().mkpath(QFileInfo(partPath).absolutePath());
    qint64 have = std::max<qint64>(0, QFileInfo(partPath).size());
    if (have > d.file().size) {
        QFile::remove(partPath); // not this file: start over
        have = 0;
    }
    const qint64 needed = d.file().size - have + kSpaceMargin;
    if (const qint64 free = QStorageInfo(m_store.root()).bytesAvailable(); free >= 0 && free < needed) {
        fail(d, tr("Not enough free space: %1 more is needed")
                    .arg(QLocale().formattedDataSize(needed - free, 1, QLocale::DataSizeTraditionalFormat)));
        return;
    }
    if (have == d.file().size) {
        onFinished(d.entry.id); // fully downloaded before: only the check is left
        return;
    }
    d.part = std::make_unique<QFile>(partPath);
    if (!d.part->open(have > 0 ? (QIODevice::WriteOnly | QIODevice::Append) : QIODevice::WriteOnly)) {
        fail(d, tr("The download could not be saved: %1").arg(d.part->errorString()));
        return;
    }
    const QUrl url = urlFor(d.entry, d.file());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, u"paraphrase"_s);
    if (have > 0) {
        request.setRawHeader("Range", "bytes=" + QByteArray::number(have) + '-');
    }
    qCInfo(lcDownloads) << d.entry.id << "downloading" << url.toString() << "from byte" << have;
    d.reply = m_network->get(request);
    d.resumeOffset = have;
    d.headerChecked = false;
    d.status.state = DownloadStatus::State::Downloading;
    d.status.received = m_store.usedBytes(d.entry);
    emitChanged(d);
    const QString id = d.entry.id;
    connect(d.reply, &QNetworkReply::readyRead, this, [this, id] { onReadyRead(id); });
    connect(d.reply, &QNetworkReply::finished, this, [this, id] { onFinished(id); });
}

bool ModelDownloader::writeAvailable(Download& d)
{
    if (!d.headerChecked) {
        d.headerChecked = true;
        const int status = d.reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // A Range request answered with the whole file (200, not 206): start the part over.
        if (d.resumeOffset > 0 && status == 200) {
            d.part->close();
            if (!d.part->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                return false;
            }
            d.status.received -= d.resumeOffset;
            d.resumeOffset = 0;
        }
        if (status >= 300) {
            return true; // an error page: nothing is written, onFinished reports it
        }
    }
    if (d.reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() >= 300) {
        d.reply->readAll();
        return true;
    }
    const QByteArray chunk = d.reply->readAll();
    qint64 done = 0;
    while (done < chunk.size()) {
        const qint64 written = d.part->write(chunk.constData() + done, chunk.size() - done);
        if (written <= 0) {
            d.status.error = d.part->errorString();
            return false;
        }
        done += written;
    }
    d.status.received += chunk.size();
    return true;
}

void ModelDownloader::onReadyRead(const QString& modelId)
{
    Download* d = find(modelId);
    if (d == nullptr || d->reply == nullptr) {
        return;
    }
    if (!writeAvailable(*d)) {
        fail(*d, tr("The download could not be saved: %1").arg(d->status.error));
        return;
    }
    emitChanged(*d);
}

void ModelDownloader::onFinished(const QString& modelId)
{
    Download* d = find(modelId);
    if (d == nullptr) {
        return;
    }
    if (d->reply != nullptr) {
        const bool saved = writeAvailable(*d);
        QNetworkReply* reply = d->reply;
        d->reply = nullptr;
        reply->deleteLater();
        d->part->close();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status >= 400) {
            fail(*d, tr("The server reported an error (%1)").arg(status));
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            if (reply->error() != QNetworkReply::OperationCanceledError) { // paused or cancelled
                fail(*d, tr("The download stopped: %1").arg(reply->errorString()));
            }
            return;
        }
        if (!saved) {
            fail(*d, tr("The download could not be saved: %1").arg(d->status.error));
            return;
        }
    }
    if (QFileInfo(m_store.partPath(d->entry, d->file())).size() != d->file().size) {
        fail(*d, tr("The download ended early; try again"));
        return;
    }
    // The sha256 of a 3 GB file takes seconds: off this thread (CODING_STANDARDS, Threads).
    d->status.state = DownloadStatus::State::Verifying;
    d->status.verified = 0;
    emitChanged(*d);
    const ModelStore store = m_store;
    const ModelEntry entry = d->entry;
    const ModelFile file = d->file();
    const auto cancel = d->cancelVerify;
    cancel->store(false);
    const QPointer<ModelDownloader> self(this);
    QThreadPool::globalInstance()->start([self, store, entry, file, cancel, modelId] {
        const auto progress = [self, modelId](qint64 n) {
            QMetaObject::invokeMethod(self, [self, modelId, n] {
                if (Download* running = self ? self->find(modelId) : nullptr; running != nullptr) {
                    running->status.verified = n;
                }
            });
        };
        const auto result = store.installPart(entry, file, progress, cancel.get());
        QMetaObject::invokeMethod(self, [self, modelId, result] {
            if (self) {
                self->onVerified(modelId, result);
            }
        });
    });
}

void ModelDownloader::onVerified(const QString& modelId, const util::Result<bool>& result)
{
    Download* d = find(modelId);
    if (d == nullptr || d->status.state != DownloadStatus::State::Verifying) {
        return; // paused or cancelled meanwhile
    }
    if (!result.ok()) {
        qCWarning(lcDownloads) << modelId << "check failed:" << result.error();
        if (result.error().contains(u"checksum"_s)) {
            d->status.received = m_store.usedBytes(d->entry);
            fail(*d, tr("The file did not match its SHA-256 fingerprint and was deleted; try again"));
        } else {
            fail(*d, tr("The file could not be checked: %1").arg(result.error()));
        }
        return;
    }
    ++d->fileIndex;
    beginFile(*d);
}

void ModelDownloader::fail(Download& d, const QString& message)
{
    stop(d);
    d.status.state = DownloadStatus::State::Failed;
    d.status.error = message;
    qCWarning(lcDownloads) << d.entry.id << "failed:" << message;
    emitChanged(d);
    startNext();
}

void ModelDownloader::stop(Download& d)
{
    d.cancelVerify->store(true);
    if (d.reply != nullptr) {
        QNetworkReply* reply = d.reply;
        d.reply = nullptr;
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
    if (d.part) {
        d.part->close();
    }
}

void ModelDownloader::pause(const QString& modelId)
{
    Download* d = find(modelId);
    if (d == nullptr || d->status.state == DownloadStatus::State::Paused) {
        return;
    }
    stop(*d);
    d->status.state = DownloadStatus::State::Paused;
    d->status.received = m_store.usedBytes(d->entry);
    emitChanged(*d);
    startNext();
}

void ModelDownloader::cancel(const QString& modelId)
{
    Download* d = find(modelId);
    if (d == nullptr) {
        return;
    }
    stop(*d);
    m_downloads.removeIf([&](const auto& x) { return x->entry.id == modelId; });
    startNext();
}

} // namespace paraphrase::models
