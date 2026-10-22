// The built-in model catalog and the models folder: the shipped models.json parses and offers
// one model per tier with its draft head; broken entries are refused by name; a download is
// installed only when its sha256 matches, and stays installed until the file changes. The
// stamps are the ones scripts/fetch_model.py writes.

#include "models/hardware_probe.h"
#include "models/model_catalog.h"
#include "models/model_store.h"
#include "models/tier_selector.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;
using paraphrase::models::HardwareInfo;
using paraphrase::models::HardwareProbe;
using paraphrase::models::ModelCatalog;
using paraphrase::models::ModelEntry;
using paraphrase::models::ModelFile;
using paraphrase::models::ModelStore;
using paraphrase::models::TierSelector;

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

/// One logical CPU in a fake /sys tree.
struct FakeCpu
{
    int core = 0;
    bool l3 = true;
};

/// A fake root with /proc/meminfo, /proc/cpuinfo and the CPU topology under /sys.
bool writeTree(const QString& root, const QList<FakeCpu>& cpus, const QByteArray& meminfo,
               const QByteArray& cpuinfo = "flags\t\t: fpu sse2 avx avx2 fma f16c avx_vnni\n")
{
    bool ok = QDir().mkpath(root + u"/proc"_s) && writeFile(root + u"/proc/meminfo"_s, meminfo) &&
              writeFile(root + u"/proc/cpuinfo"_s, cpuinfo);
    for (qsizetype i = 0; i < cpus.size(); ++i) {
        const QString base = root + u"/sys/devices/system/cpu/cpu%1"_s.arg(i);
        ok = ok && QDir().mkpath(base + u"/topology"_s) && QDir().mkpath(base + u"/cache/index2"_s) &&
             writeFile(base + u"/topology/core_id"_s, QByteArray::number(cpus.at(i).core)) &&
             writeFile(base + u"/topology/physical_package_id"_s, "0") &&
             writeFile(base + u"/cache/index2/level"_s, "2");
        if (cpus.at(i).l3) {
            ok = ok && QDir().mkpath(base + u"/cache/index3"_s) &&
                 writeFile(base + u"/cache/index3/level"_s, "3");
        }
    }
    return ok;
}

/// Machine B: Core Ultra 7 255U, 2 P-cores with hyper-threads, 8 E-cores, 2 LP E-cores.
QList<FakeCpu> ultra255u()
{
    QList<FakeCpu> cpus = {{.core = 8}, {.core = 8}, {.core = 12}, {.core = 12}};
    for (int i = 0; i < 8; ++i) {
        cpus.append({.core = i});
    }
    cpus.append({.core = 32, .l3 = false});
    cpus.append({.core = 33, .l3 = false});
    return cpus;
}

HardwareInfo withMemoryGb(double gb)
{
    HardwareInfo hw;
    hw.totalMemory = static_cast<qint64>(gb * 1024 * 1024 * 1024);
    return hw;
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

    void probeLeavesOutLowPowerCores()
    {
        const QTemporaryDir dir;
        QVERIFY(
            writeTree(dir.path(), ultra255u(), "MemTotal:       32294020 kB\nMemAvailable:   18490300 kB\n"));
        const HardwareInfo hw = HardwareProbe::probe(dir.path(), 14);
        QCOMPARE(hw.logicalCpus, 14);
        QCOMPARE(hw.physicalCores, 12);
        QCOMPARE(hw.threads, 10); // the fastest in the M0 sweep
        QCOMPARE(hw.totalMemory, qint64{32294020} * 1024);
        QCOMPARE(hw.availableMemory, qint64{18490300} * 1024);
        QCOMPARE(hw.cpuFeatures, (QStringList{u"avx"_s, u"avx2"_s, u"fma"_s, u"f16c"_s, u"avx_vnni"_s}));
        // Pinned to 4 CPUs, as M0 emulated machine A.
        QCOMPARE(HardwareProbe::probe(dir.path(), 4).threads, 4);
    }

    void probeCountsPhysicalCores()
    {
        const QTemporaryDir dir;
        // 4 cores with hyper-threads; no cache information at all (some VMs) counts the same.
        const QList<FakeCpu> cpus = {
            {.core = 0}, {.core = 1}, {.core = 2}, {.core = 3},
            {.core = 0}, {.core = 1}, {.core = 2}, {.core = 3},
        };
        QVERIFY(writeTree(dir.path(), cpus, "MemTotal: 8000000 kB\n"));
        QCOMPARE(HardwareProbe::probe(dir.path(), 8).threads, 4);
        const QTemporaryDir vm;
        QVERIFY(writeTree(vm.path(), {{.core = 0, .l3 = false}, {.core = 1, .l3 = false}},
                          "MemTotal: 8000000 kB\n"));
        QCOMPARE(HardwareProbe::probe(vm.path(), 2).threads, 2);
    }

    void probeHonoursACgroupMemoryLimit()
    {
        const QTemporaryDir dir;
        QVERIFY(writeTree(dir.path(), ultra255u(), "MemTotal: 32294020 kB\nMemAvailable: 18490300 kB\n"));
        QVERIFY(QDir().mkpath(dir.path() + u"/proc/self"_s));
        QVERIFY(writeFile(dir.path() + u"/proc/self/cgroup"_s, "0::/user.slice/emulated-a.scope\n"));
        QVERIFY(QDir().mkpath(dir.path() + u"/sys/fs/cgroup/user.slice/emulated-a.scope"_s));
        QVERIFY(writeFile(dir.path() + u"/sys/fs/cgroup/user.slice/memory.max"_s, "max\n"));
        QVERIFY(writeFile(dir.path() + u"/sys/fs/cgroup/user.slice/emulated-a.scope/memory.max"_s,
                          "8589934592\n"));
        const HardwareInfo hw = HardwareProbe::probe(dir.path(), 4);
        QCOMPARE(hw.totalMemory, qint64{8589934592});
        QCOMPARE(hw.totalMemoryGb(), 8.0);
    }

    void probesThisComputer()
    {
        const HardwareInfo hw = HardwareProbe::probe();
        QVERIFY(hw.totalMemory > 0);
        QVERIFY(hw.threads >= 1 && hw.threads <= hw.logicalCpus);
        qInfo("%s: %.1f GB, %d threads of %d CPUs", qPrintable(hw.cpuModel), hw.totalMemoryGb(), hw.threads,
              hw.logicalCpus);
    }

    void qualityTierNeedsSixteenGb()
    {
        const ModelCatalog catalog = ModelCatalog::builtIn();
        const QSet<QString> both = {u"gemma-4-e2b-qat-q4_0"_s, u"gemma-4-e4b-qat-q4_0"_s};
        const QString fast = u"gemma-4-e2b-qat-q4_0"_s;
        const QString quality = u"gemma-4-e4b-qat-q4_0"_s;

        const TierSelector small(catalog, withMemoryGb(7.6)); // an "8 GB" laptop
        const auto offers = small.offers();
        QCOMPARE(offers.size(), 2);
        QCOMPARE(offers.at(0).model->id, fast);
        QVERIFY(offers.at(0).fits && offers.at(0).recommended);
        QVERIFY(!offers.at(1).fits && !offers.at(1).recommended);
        QCOMPARE(small.modelFor(u"quality"_s, both)->id, fast); // rewrite runs on the fast tier

        const TierSelector big(catalog, withMemoryGb(15.3)); // a "16 GB" one
        QVERIFY(big.offers().at(1).fits);
        QCOMPARE(big.modelFor(u"quality"_s, both)->id, quality);
        QCOMPARE(big.modelFor(u"fast"_s, both)->id, fast);
        QCOMPARE(big.modelFor(u"any"_s, both)->id, fast);
        QCOMPARE(big.modelFor(u"quality"_s, {fast})->id, fast);
        QCOMPARE(big.modelFor(u"fast"_s, {quality})->id, quality); // only the quality one installed
        QVERIFY(big.modelFor(u"fast"_s, {}) == nullptr);
        QVERIFY(small.modelFor(u"fast"_s, {quality}) == nullptr); // installed, but too large

        // Below its guide the fast model still runs (ADR-003): it is the only choice.
        QCOMPARE(TierSelector(catalog, withMemoryGb(5.8)).modelFor(u"fast"_s, both)->id, fast);
    }
};

QTEST_GUILESS_MAIN(TestModels)
#include "tst_models.moc"
