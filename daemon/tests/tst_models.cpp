// The built-in model catalog and the models folder: the shipped models.json parses and offers
// one model per tier with its draft head; broken entries are refused by name; a download is
// installed only when its sha256 matches, and stays installed until the file changes. The
// stamps are the ones scripts/fetch_model.py writes.

#include "models/model_catalog.h"
#include "models/model_store.h"

#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;
using paraphrase::models::ModelCatalog;
using paraphrase::models::ModelEntry;
using paraphrase::models::ModelFile;
using paraphrase::models::ModelStore;

namespace {

QJsonObject validEntry()
{
    return QJsonObject{
        {u"id"_s, u"tiny-model"_s},
        {u"name"_s, u"Tiny"_s},
        {u"role"_s, u"model"_s},
        {u"family"_s, u"test"_s},
        {u"hf_repo"_s, u"owner/tiny-GGUF"_s},
        {u"revision"_s, QString(40, u'a')},
        {
            u"files"_s,
            QJsonArray{
                QJsonObject{
                    {u"file"_s, u"tiny.gguf"_s},
                    {u"size"_s, 5},
                    {u"sha256"_s, QString(64, u'b')},
                },
            },
        },
        {u"ctx"_s, 4096},
        {u"licence"_s, u"MIT"_s},
        {u"licence_url"_s, u"https://example.org/licence"_s},
        {u"tiers"_s, QJsonArray{u"fast"_s}},
        {u"ram_required"_s, 8},
        {u"memory_gb"_s, 1.5},
    };
}

QByteArray catalogJson(const QJsonArray& models, int schema = ModelCatalog::kSchemaVersion)
{
    return QJsonDocument(QJsonObject{{u"schema_version"_s, schema}, {u"models"_s, models}}).toJson();
}

bool writeFile(const QString& path, const QByteArray& data)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write(data) == data.size();
}

ModelEntry entryFor(const QByteArray& content)
{
    ModelEntry e;
    e.id = u"tiny-model"_s;
    e.files = {
        ModelFile{
            .file = u"tiny.gguf"_s,
            .size = content.size(),
            .sha256 =
                QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex()),
        },
    };
    return e;
}

} // namespace

class TestModels : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void shippedCatalogOffersOneModelPerTier()
    {
        const ModelCatalog catalog = ModelCatalog::builtIn();
        const ModelEntry* fast = catalog.modelForTier(u"fast"_s);
        const ModelEntry* quality = catalog.modelForTier(u"quality"_s);
        QVERIFY(fast != nullptr && quality != nullptr);
        QCOMPARE(fast->id, u"gemma-4-e2b-qat-q4_0"_s);
        QCOMPARE(quality->id, u"gemma-4-e4b-qat-q4_0"_s);
        QCOMPARE(fast->ramRequiredGb, 8.0);
        QCOMPARE(quality->ramRequiredGb, 16.0);
        const ModelEntry* draft = catalog.draftFor(fast->id);
        QVERIFY(draft != nullptr);
        QCOMPARE(draft->id, u"gemma-4-e2b-mtp-q4_0"_s);
        QVERIFY(catalog.draftFor(quality->id) != nullptr);
        QVERIFY(!catalog.find(u"sarvam-30b-q4km"_s)->isOffered()); // evaluation only
        QCOMPARE(fast->url(fast->files.front()),
                 QUrl(u"https://huggingface.co/google/gemma-4-E2B-it-qat-q4_0-gguf/resolve/"
                      "675cff42a74c774d6cb76f76d8eacb49b48c9b93/gemma-4-E2B_q4_0-it.gguf"_s));
    }

    void refusesBrokenEntries()
    {
        struct Case
        {
            const char* what;
            QString key;
            QJsonValue value;
            QString error;
        };
        const auto file = [](const QString& name, const QString& sha) {
            return QJsonValue(QJsonArray{QJsonObject{{u"file"_s, name}, {u"size"_s, 5}, {u"sha256"_s, sha}}});
        };
        const QList<Case> cases = {
            {
                .what = "path in a file name",
                .key = u"files"_s,
                .value = file(u"../evil.gguf"_s, QString(64, u'b')),
                .error = u"bad file name"_s,
            },
            {
                .what = "folder in a file name",
                .key = u"files"_s,
                .value = file(u"sub/x.gguf"_s, QString(64, u'b')),
                .error = u"bad file name"_s,
            },
            {
                .what = "short sha256",
                .key = u"files"_s,
                .value = file(u"x.gguf"_s, u"abc"_s),
                .error = u"sha256"_s,
            },
            {
                .what = "upper-case sha256",
                .key = u"files"_s,
                .value = file(u"x.gguf"_s, QString(64, u'B')),
                .error = u"sha256"_s,
            },
            {
                .what = "branch, not a revision",
                .key = u"revision"_s,
                .value = QJsonValue(u"main"_s),
                .error = u"40 hex"_s,
            },
            {
                .what = "repository with a path",
                .key = u"hf_repo"_s,
                .value = QJsonValue(u"a/b/../c"_s),
                .error = u"hf_repo"_s,
            },
            {
                .what = "unknown role",
                .key = u"role"_s,
                .value = QJsonValue(u"shipping"_s),
                .error = u"unknown role"_s,
            },
            {
                .what = "unknown tier",
                .key = u"tiers"_s,
                .value = QJsonValue(QJsonArray{u"turbo"_s}),
                .error = u"unknown tier"_s,
            },
            {
                .what = "model without tiers",
                .key = u"tiers"_s,
                .value = QJsonValue(QJsonArray{}),
                .error = u"needs tiers"_s,
            },
            {
                .what = "plain-http licence",
                .key = u"licence_url"_s,
                .value = QJsonValue(u"http://x.org"_s),
                .error = u"https"_s,
            },
        };
        for (const auto& c : cases) {
            QJsonObject entry = validEntry();
            entry.insert(c.key, c.value);
            const auto parsed = ModelCatalog::parse(catalogJson({entry}));
            QVERIFY2(!parsed.ok(), c.what);
            QVERIFY2(parsed.error().contains(u"tiny-model"_s), qPrintable(parsed.error()));
            QVERIFY2(parsed.error().contains(c.error), qPrintable(parsed.error()));
        }
    }

    void refusesBrokenCatalogs()
    {
        QVERIFY(ModelCatalog::parse(catalogJson({validEntry()})).ok());
        QVERIFY(!ModelCatalog::parse(catalogJson({validEntry()}, 1)).ok());
        QVERIFY(!ModelCatalog::parse(catalogJson({validEntry(), validEntry()})).ok());
        QVERIFY(!ModelCatalog::parse("[]").ok());
        QJsonObject draft = validEntry();
        draft.insert(u"id"_s, u"tiny-draft"_s);
        draft.insert(u"role"_s, u"draft"_s);
        draft.insert(u"draft_for"_s, QJsonArray{u"missing-model"_s});
        const auto parsed = ModelCatalog::parse(catalogJson({validEntry(), draft}));
        QVERIFY(!parsed.ok());
        QVERIFY(parsed.error().contains(u"missing-model"_s));
    }

    void installsOnlyAMatchingDownload()
    {
        const QTemporaryDir dir;
        const ModelStore store(dir.path());
        const QByteArray content = "hello";
        const ModelEntry entry = entryFor(content);
        const ModelFile& file = entry.files.front();
        QVERIFY(QDir().mkpath(dir.path() + u"/tiny-model"_s));
        QVERIFY(!store.isInstalled(entry));

        // Short: kept for a resume, not checked.
        QVERIFY(writeFile(store.partPath(entry, file), "hel"));
        auto result = store.installPart(entry, file);
        QVERIFY(!result.ok());
        QVERIFY(result.error().contains(u"size mismatch"_s));
        QVERIFY(QFile::exists(store.partPath(entry, file)));

        // Right size, wrong bytes: deleted.
        QVERIFY(writeFile(store.partPath(entry, file), "jello"));
        result = store.installPart(entry, file);
        QVERIFY(!result.ok());
        QVERIFY(result.error().contains(u"checksum mismatch"_s));
        QVERIFY(!QFile::exists(store.partPath(entry, file)));

        QVERIFY(writeFile(store.partPath(entry, file), content));
        qint64 hashed = 0;
        result = store.installPart(entry, file, [&](qint64 n) { hashed = n; });
        QVERIFY2(result.ok(), qPrintable(result.error()));
        QCOMPARE(hashed, content.size());
        QVERIFY(store.isInstalled(entry));
        QVERIFY(!QFile::exists(store.partPath(entry, file)));
        QCOMPARE(store.usedBytes(entry), content.size());

        // A changed file is no longer trusted.
        QVERIFY(writeFile(store.path(entry, file), "jello"));
        QVERIFY(!store.isInstalled(entry));

        QVERIFY(store.remove(entry).value());
        QVERIFY(!QDir(dir.path() + u"/tiny-model"_s).exists());
        QVERIFY(!store.remove(entry).value());
    }

    void cancelKeepsThePart()
    {
        const QTemporaryDir dir;
        const ModelStore store(dir.path());
        const ModelEntry entry = entryFor("hello");
        QVERIFY(QDir().mkpath(dir.path() + u"/tiny-model"_s));
        QVERIFY(writeFile(store.partPath(entry, entry.files.front()), "hello"));
        const std::atomic<bool> cancel{true};
        const auto result = store.installPart(entry, entry.files.front(), {}, &cancel);
        QVERIFY(!result.ok());
        QVERIFY(QFile::exists(store.partPath(entry, entry.files.front())));
    }

    void readsStampsFromFetchModel()
    {
        // Models fetched with scripts/fetch_model.py (development) count as installed.
        const ModelStore store(QStringLiteral(PARAPHRASE_REPO_ROOT) + u"/models"_s);
        const ModelEntry* fast = ModelCatalog::builtIn().modelForTier(u"fast"_s);
        if (!QFile::exists(store.modelPath(*fast) + u".sha256"_s)) {
            QSKIP("the fast tier model was not fetched (make fetch)");
        }
        QVERIFY(store.isInstalled(*fast));
    }

    void defaultRootFollowsThePackage()
    {
        qputenv("PARAPHRASE_MODELS_DIR", "/tmp/dev-models");
        QCOMPARE(ModelStore::defaultRoot(), u"/tmp/dev-models"_s);
        qunsetenv("PARAPHRASE_MODELS_DIR");
        qputenv("SNAP_USER_COMMON", "/home/u/snap/paraphrase/common");
        QCOMPARE(ModelStore::defaultRoot(), u"/home/u/snap/paraphrase/common/models"_s);
        qunsetenv("SNAP_USER_COMMON");
        QVERIFY(ModelStore::defaultRoot().endsWith(u"/paraphrase/models"_s));
    }
};

QTEST_GUILESS_MAIN(TestModels)
#include "tst_models.moc"
