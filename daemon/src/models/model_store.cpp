#include "model_store.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include <sys/stat.h>

using namespace Qt::StringLiterals;

namespace paraphrase::models {
namespace {

constexpr qint64 kChunk = qint64{4} * 1024 * 1024;

/// st_mtime in nanoseconds, as Python's os.stat().st_mtime_ns; -1 when the file is missing.
qint64 mtimeNs(const QString& path)
{
    struct stat st{};
    if (::stat(QFile::encodeName(path).constData(), &st) != 0) {
        return -1;
    }
    return (static_cast<qint64>(st.st_mtim.tv_sec) * 1'000'000'000) + st.st_mtim.tv_nsec;
}

} // namespace

ModelStore::ModelStore(QString root)
    : m_root(std::move(root))
{}

QString ModelStore::defaultRoot()
{
    if (const QString dir = qEnvironmentVariable("PARAPHRASE_MODELS_DIR"); !dir.isEmpty()) {
        return dir;
    }
    if (const QString snap = qEnvironmentVariable("SNAP_USER_COMMON"); !snap.isEmpty()) {
        return snap + u"/models"_s;
    }
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/paraphrase/models"_s;
}

QString ModelStore::path(const ModelEntry& entry, const ModelFile& file) const
{
    return m_root + u'/' + entry.id + u'/' + file.file;
}

QString ModelStore::partPath(const ModelEntry& entry, const ModelFile& file) const
{
    return path(entry, file) + u".part"_s;
}

QString ModelStore::modelPath(const ModelEntry& entry) const
{
    return path(entry, entry.files.front());
}

QString ModelStore::stampPath(const QString& path)
{
    return path + u".sha256"_s;
}

QString ModelStore::stampText(const QString& path, const ModelFile& file)
{
    return u"%1 %2 %3"_s.arg(file.sha256).arg(file.size).arg(mtimeNs(path));
}

bool ModelStore::isVerified(const ModelEntry& entry, const ModelFile& file) const
{
    const QString p = path(entry, file);
    if (QFileInfo(p).size() != file.size) {
        return false;
    }
    QFile stamp(stampPath(p));
    if (!stamp.open(QIODevice::ReadOnly)) {
        return false;
    }
    return QString::fromUtf8(stamp.readAll()).trimmed() == stampText(p, file);
}

bool ModelStore::isInstalled(const ModelEntry& entry) const
{
    return std::ranges::all_of(entry.files, [&](const ModelFile& f) { return isVerified(entry, f); });
}

qint64 ModelStore::usedBytes(const ModelEntry& entry) const
{
    qint64 total = 0;
    for (const auto& f : entry.files) {
        total += std::max<qint64>(0, QFileInfo(path(entry, f)).size());
        total += std::max<qint64>(0, QFileInfo(partPath(entry, f)).size());
    }
    return total;
}

QString ModelStore::sha256Of(const QString& path, const std::function<void(qint64)>& progress,
                             const std::atomic<bool>* cancel)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    qint64 done = 0;
    while (!file.atEnd()) {
        if (cancel != nullptr && cancel->load()) {
            return {};
        }
        const QByteArray chunk = file.read(kChunk);
        if (chunk.isEmpty()) {
            return {}; // a read error
        }
        hash.addData(chunk);
        done += chunk.size();
        if (progress) {
            progress(done);
        }
    }
    return QString::fromLatin1(hash.result().toHex());
}

util::Result<bool> ModelStore::installPart(const ModelEntry& entry, const ModelFile& file,
                                           const std::function<void(qint64)>& progress,
                                           const std::atomic<bool>* cancel) const
{
    using R = util::Result<bool>;
    const QString part = partPath(entry, file);
    const qint64 size = QFileInfo(part).size();
    if (size != file.size) {
        return R::failure(
            u"size mismatch: %1 has %2 bytes, the catalog says %3"_s.arg(file.file).arg(size).arg(file.size));
    }
    const QString digest = sha256Of(part, progress, cancel);
    if (cancel != nullptr && cancel->load()) {
        return R::failure(u"cancelled"_s);
    }
    if (digest.isEmpty()) {
        return R::failure(u"cannot read %1"_s.arg(part));
    }
    if (digest != file.sha256) {
        QFile::remove(part);
        return R::failure(
            u"checksum mismatch: %1 is not the file the catalog pins; it was deleted"_s.arg(file.file));
    }
    const QString dest = path(entry, file);
    QFile::remove(dest);
    QFile::remove(stampPath(dest));
    if (!QFile::rename(part, dest)) {
        return R::failure(u"cannot move %1 into place"_s.arg(file.file));
    }
    QFile stamp(stampPath(dest));
    if (!stamp.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return R::failure(u"cannot write %1"_s.arg(stamp.fileName()));
    }
    stamp.write((stampText(dest, file) + u'\n').toUtf8());
    return R::success(true);
}

util::Result<bool> ModelStore::remove(const ModelEntry& entry) const
{
    QDir dir(m_root + u'/' + entry.id);
    if (!dir.exists()) {
        return util::Result<bool>::success(false);
    }
    if (!dir.removeRecursively()) {
        return util::Result<bool>::failure(u"cannot remove %1"_s.arg(dir.path()));
    }
    return util::Result<bool>::success(true);
}

} // namespace paraphrase::models
