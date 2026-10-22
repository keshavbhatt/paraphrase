// Model downloads against a local test server: a model is installed only after its sha256
// matches; downloads resume with Range across pauses and restarts, start over when the server
// ignores Range, follow redirects, and report errors without losing what arrived.

#include "models/model_downloader.h"
#include "test_http_server.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;
using paraphrase::models::DownloadStatus;
using paraphrase::models::ModelDownloader;
using paraphrase::models::ModelEntry;
using paraphrase::models::ModelFile;
using paraphrase::models::ModelStore;
using State = paraphrase::models::DownloadStatus::State;

namespace {

constexpr int kTimeoutMs = 10'000;
constexpr qsizetype kMiB = qsizetype{1024} * 1024;

QByteArray randomBytes(qsizetype n, quint32 seed)
{
    QByteArray out(n, Qt::Uninitialized);
    QRandomGenerator rng(seed);
    for (qsizetype i = 0; i < n; ++i) {
        out[i] = static_cast<char>(rng.bounded(256));
    }
    return out;
}

ModelFile fileFor(const QString& name, const QByteArray& content)
{
    return {
        .file = name,
        .size = content.size(),
        .sha256 = QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex()),
    };
}

ModelEntry entryWith(const QList<ModelFile>& files)
{
    ModelEntry e;
    e.id = u"tiny-model"_s;
    e.files = files;
    return e;
}

bool writeFile(const QString& path, const QByteArray& data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write(data) == data.size();
}

State lastState(const QSignalSpy& spy)
{
    return spy.isEmpty() ? State::Waiting : spy.last().at(0).value<DownloadStatus>().state;
}

} // namespace

class TestDownloader : public QObject
{
    Q_OBJECT

private:
    TestHttpServer* m_server = nullptr;
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<ModelDownloader> m_downloader;
    QByteArray m_content = randomBytes((3 * kMiB) + 17, 42);

    [[nodiscard]] ModelStore store() const { return ModelStore(m_dir->path()); }

private Q_SLOTS:
    void init()
    {
        m_server = new TestHttpServer(this);
        m_dir = std::make_unique<QTemporaryDir>();
        m_downloader = std::make_unique<ModelDownloader>(
            store(), nullptr,
            [this](const ModelEntry&, const ModelFile& f) { return m_server->urlFor(u'/' + f.file); });
    }

    void cleanup()
    {
        m_downloader.reset();
        delete m_server;
        m_server = nullptr;
    }

    void downloadsChecksAndInstalls()
    {
        m_server->addRoute(u"/m.gguf"_s, {.body = m_content});
        const ModelEntry entry = entryWith({fileFor(u"m.gguf"_s, m_content)});
        QSignalSpy installed(m_downloader.get(), &ModelDownloader::installed);
        const QSignalSpy changed(m_downloader.get(), &ModelDownloader::changed);
        m_downloader->start(entry);
        QTRY_COMPARE_WITH_TIMEOUT(installed.size(), 1, kTimeoutMs);
        QCOMPARE(installed.first().at(0).toString(), entry.id);
        QVERIFY(store().isInstalled(entry));
        QVERIFY(!QFile::exists(store().partPath(entry, entry.files.front())));
        QCOMPARE(lastState(changed), State::Installed);
        QVERIFY(m_downloader->downloads().isEmpty());
        // Starting an installed model asks the server for nothing.
        m_downloader->start(entry);
        QTRY_COMPARE_WITH_TIMEOUT(installed.size(), 2, kTimeoutMs);
        QCOMPARE(m_server->requests(u"/m.gguf"_s), 1);
    }

    void resumesWithRange()
    {
        m_server->addRoute(u"/m.gguf"_s, {.body = m_content, .supportsRange = true});
        const ModelEntry entry = entryWith({fileFor(u"m.gguf"_s, m_content)});
        QVERIFY(writeFile(store().partPath(entry, entry.files.front()), m_content.left(1000)));
        const QSignalSpy installed(m_downloader.get(), &ModelDownloader::installed);
        m_downloader->start(entry);
        QTRY_COMPARE_WITH_TIMEOUT(installed.size(), 1, kTimeoutMs);
        QVERIFY(m_server->sawRangeRequest(u"/m.gguf"_s));
        QVERIFY(store().isInstalled(entry));
    }

    void startsOverWhenRangeIsIgnored()
    {
        m_server->addRoute(u"/m.gguf"_s, {.body = m_content, .supportsRange = false});
        const ModelEntry entry = entryWith({fileFor(u"m.gguf"_s, m_content)});
        QVERIFY(writeFile(store().partPath(entry, entry.files.front()), QByteArray(1000, 'x')));
        const QSignalSpy installed(m_downloader.get(), &ModelDownloader::installed);
        m_downloader->start(entry);
        QTRY_COMPARE_WITH_TIMEOUT(installed.size(), 1, kTimeoutMs);
        QVERIFY(store().isInstalled(entry));
    }

    void deletesAFileThatFailsTheCheck()
    {
        m_server->addRoute(u"/m.gguf"_s, {.body = m_content});
        ModelFile wrong = fileFor(u"m.gguf"_s, m_content);
        wrong.sha256 = QString(64, u'0');
        const ModelEntry entry = entryWith({wrong});
        QSignalSpy changed(m_downloader.get(), &ModelDownloader::changed);
        m_downloader->start(entry);
        QTRY_COMPARE_WITH_TIMEOUT(lastState(changed), State::Failed, kTimeoutMs);
        QVERIFY(changed.last().at(0).value<DownloadStatus>().error.contains(u"SHA-256"_s));
        QVERIFY(!QFile::exists(store().partPath(entry, entry.files.front())));
        QVERIFY(!store().isInstalled(entry));
    }

    void reportsServerErrors()
    {
        const ModelEntry entry = entryWith({fileFor(u"missing.gguf"_s, m_content)});
        QSignalSpy changed(m_downloader.get(), &ModelDownloader::changed);
        m_downloader->start(entry);
        QTRY_COMPARE_WITH_TIMEOUT(lastState(changed), State::Failed, kTimeoutMs);
        QVERIFY(changed.last().at(0).value<DownloadStatus>().error.contains(u"404"_s));
        QCOMPARE(QFileInfo(store().partPath(entry, entry.files.front())).size(), 0); // no error page saved
    }

    void pausesAndResumes()
    {
        m_server->addRoute(u"/m.gguf"_s,
                           {.body = m_content, .supportsRange = true, .splitAt = kMiB, .pauseMs = 2000});
        const ModelEntry entry = entryWith({fileFor(u"m.gguf"_s, m_content)});
        QSignalSpy changed(m_downloader.get(), &ModelDownloader::changed);
        const QSignalSpy installed(m_downloader.get(), &ModelDownloader::installed);
        m_downloader->start(entry);
        QTRY_VERIFY_WITH_TIMEOUT(QFileInfo(store().partPath(entry, entry.files.front())).size() >= kMiB,
                                 kTimeoutMs);
        m_downloader->pause(entry.id);
        QCOMPARE(lastState(changed), State::Paused);
        const qint64 kept = QFileInfo(store().partPath(entry, entry.files.front())).size();
        QVERIFY(kept > 0 && kept < m_content.size());
        QCOMPARE(changed.last().at(0).value<DownloadStatus>().received, kept);

        // A new downloader is a restart of the daemon: the part file is all it has.
        m_downloader = std::make_unique<ModelDownloader>(
            store(), nullptr,
            [this](const ModelEntry&, const ModelFile& f) { return m_server->urlFor(u'/' + f.file); });
        m_server->addRoute(u"/m.gguf"_s, {.body = m_content, .supportsRange = true});
        const QSignalSpy resumed(m_downloader.get(), &ModelDownloader::installed);
        m_downloader->start(entry);
        QTRY_COMPARE_WITH_TIMEOUT(resumed.size(), 1, kTimeoutMs);
        QVERIFY(m_server->sawRangeRequest(u"/m.gguf"_s));
        QVERIFY(store().isInstalled(entry));
    }

    void followsRedirects()
    {
        // Hugging Face answers /resolve/ with a redirect to its file host.
        m_server->addRoute(u"/m.gguf"_s, {.redirectTo = u"/cdn/m.gguf"_s});
        m_server->addRoute(u"/cdn/m.gguf"_s, {.body = m_content});
        const ModelEntry entry = entryWith({fileFor(u"m.gguf"_s, m_content)});
        const QSignalSpy installed(m_downloader.get(), &ModelDownloader::installed);
        m_downloader->start(entry);
        QTRY_COMPARE_WITH_TIMEOUT(installed.size(), 1, kTimeoutMs);
    }

    void downloadsOnlyMissingFiles()
    {
        const QByteArray second = randomBytes(5000, 7);
        m_server->addRoute(u"/a.gguf"_s, {.body = m_content});
        m_server->addRoute(u"/b.gguf"_s, {.body = second});
        const ModelEntry entry = entryWith({fileFor(u"a.gguf"_s, m_content), fileFor(u"b.gguf"_s, second)});
        QVERIFY(writeFile(store().partPath(entry, entry.files.at(0)), m_content));
        QVERIFY(store().installPart(entry, entry.files.at(0)).ok());
        const QSignalSpy installed(m_downloader.get(), &ModelDownloader::installed);
        m_downloader->start(entry);
        QTRY_COMPARE_WITH_TIMEOUT(installed.size(), 1, kTimeoutMs);
        QCOMPARE(m_server->requests(u"/a.gguf"_s), 0);
        QCOMPARE(m_server->requests(u"/b.gguf"_s), 1);
        QVERIFY(store().isInstalled(entry));
    }

    void downloadsFromHuggingFace()
    {
        // The real thing: TLS, Hugging Face's redirect to its file host, the pinned revision.
        // Opt-in, so the suite stays offline (CI, and the rule that tests need no network).
        if (qEnvironmentVariable("PARAPHRASE_NETWORK_TESTS") != u"1"_s) {
            QSKIP("set PARAPHRASE_NETWORK_TESTS=1 to download the E2B draft head (59 MB)");
        }
        const auto catalog = paraphrase::models::ModelCatalog::builtIn();
        const ModelEntry* head = catalog.find(u"gemma-4-e2b-mtp-q4_0"_s);
        QVERIFY(head != nullptr);
        ModelDownloader downloader(store());
        const QSignalSpy installed(&downloader, &ModelDownloader::installed);
        QSignalSpy changed(&downloader, &ModelDownloader::changed);
        downloader.start(*head);
        QTRY_VERIFY_WITH_TIMEOUT(installed.size() == 1 || lastState(changed) == State::Failed, 600'000);
        QVERIFY2(installed.size() == 1, qPrintable(changed.last().at(0).value<DownloadStatus>().error));
        QVERIFY(store().isInstalled(*head));
    }

    void usesHttpsFromTheCatalog()
    {
        ModelEntry entry = entryWith({fileFor(u"m.gguf"_s, m_content)});
        entry.hfRepo = u"google/gemma-4-E2B-it-qat-q4_0-gguf"_s;
        entry.revision = QString(40, u'a');
        QCOMPARE(entry.url(entry.files.front()).scheme(), u"https"_s);
    }
};

QTEST_GUILESS_MAIN(TestDownloader)
#include "tst_downloader.moc"
