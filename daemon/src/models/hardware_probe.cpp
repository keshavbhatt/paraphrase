#include "hardware_probe.h"

#include <QDir>
#include <QFile>
#include <QSet>

#include <algorithm>
#include <sched.h>

using namespace Qt::StringLiterals;

namespace paraphrase::models {
namespace {

constexpr double kBytesPerGb = 1024.0 * 1024.0 * 1024.0;

QString readText(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromUtf8(f.readAll()).trimmed();
}

/// A "<key>: <number> kB" line of /proc/meminfo, in bytes; 0 when missing.
qint64 meminfo(const QString& text, QStringView key)
{
    for (const QStringView line : QStringView(text).split(u'\n')) {
        if (line.startsWith(key) && line.size() > key.size() && line.at(key.size()) == u':') {
            const auto fields = line.mid(key.size() + 1).trimmed().split(u' ');
            return fields.front().toLongLong() * 1024;
        }
    }
    return 0;
}

/// The memory limit of this process's cgroup (v2) and its parents, or 0 for none. A desktop
/// session has none; a container or `systemd-run -p MemoryMax=` does (M0's emulated A).
qint64 cgroupLimit(const QString& root)
{
    QString group;
    for (const QStringView line : QStringView(readText(root + u"proc/self/cgroup"_s)).split(u'\n')) {
        if (line.startsWith(u"0::")) {
            group = line.mid(3).toString();
        }
    }
    if (group.isEmpty()) {
        return 0;
    }
    qint64 limit = 0;
    for (QString g = group; !g.isEmpty(); g = g.section(u'/', 0, -2)) {
        const QString value = readText(root + u"sys/fs/cgroup"_s + g + u"/memory.max"_s);
        bool ok = false;
        const qint64 bytes = value.toLongLong(&ok);
        if (ok && bytes > 0 && (limit == 0 || bytes < limit)) {
            limit = bytes;
        }
    }
    return limit;
}

/// The CPU flags llama.cpp picks its kernels by (x86; ARM reports its own names).
QStringList cpuFeatures(const QString& cpuinfo)
{
    static const QStringList kInteresting = {
        u"avx"_s,      u"avx2"_s,        u"fma"_s,         u"f16c"_s,     u"avx_vnni"_s, u"avx512f"_s,
        u"avx512bw"_s, u"avx512_vnni"_s, u"avx512_bf16"_s, u"amx_tile"_s, u"asimd"_s,    u"sve"_s,
    };
    QSet<QString> present;
    for (const QStringView line : QStringView(cpuinfo).split(u'\n')) {
        if (line.startsWith(u"flags") || line.startsWith(u"Features")) {
            for (const QStringView flag : line.mid(line.indexOf(u':') + 1).split(u' ', Qt::SkipEmptyParts)) {
                present.insert(flag.toString());
            }
            break;
        }
    }
    QStringList out;
    for (const QString& f : kInteresting) {
        if (present.contains(f)) {
            out.append(f);
        }
    }
    return out;
}

QString cpuModel(const QString& cpuinfo)
{
    for (const QStringView line : QStringView(cpuinfo).split(u'\n')) {
        if (line.startsWith(u"model name")) {
            return line.mid(line.indexOf(u':') + 1).trimmed().toString();
        }
    }
    return {};
}

int affinityCpus()
{
    cpu_set_t set;
    CPU_ZERO(&set);
    if (sched_getaffinity(0, sizeof(set), &set) != 0) {
        return 0;
    }
    return CPU_COUNT(&set);
}

struct Cpu
{
    QString core; ///< "<package>/<core id>"
    bool hasL3 = false;
};

QList<Cpu> readCpus(const QString& root)
{
    QList<Cpu> cpus;
    const QDir dir(root + u"sys/devices/system/cpu"_s);
    const QStringList names = dir.entryList({u"cpu[0-9]*"_s}, QDir::Dirs);
    for (const QString& name : names) {
        const QString base = dir.filePath(name);
        const QString coreId = readText(base + u"/topology/core_id"_s);
        if (coreId.isEmpty()) {
            continue; // offline
        }
        Cpu cpu{.core = readText(base + u"/topology/physical_package_id"_s) + u'/' + coreId, .hasL3 = false};
        const QStringList caches = QDir(base + u"/cache"_s).entryList({u"index*"_s}, QDir::Dirs);
        cpu.hasL3 = std::ranges::any_of(caches, [&](const QString& index) {
            return readText(base + u"/cache/"_s + index + u"/level"_s) == u"3"_s;
        });
        cpus.append(cpu);
    }
    return cpus;
}

} // namespace

double HardwareInfo::totalMemoryGb() const
{
    return static_cast<double>(totalMemory) / kBytesPerGb;
}

int HardwareProbe::threadsFor(const QString& root, int usableCpus, int* physicalCores, int* logicalCpus)
{
    const QList<Cpu> cpus = readCpus(root.endsWith(u'/') ? root : root + u'/');
    // Low-power cores sit outside the L3 the others share; leave them out when there are both.
    const bool someWithL3 = std::ranges::any_of(cpus, [](const Cpu& c) { return c.hasL3; });
    QSet<QString> all;
    QSet<QString> fast;
    for (const Cpu& c : cpus) {
        all.insert(c.core);
        if (c.hasL3 || !someWithL3) {
            fast.insert(c.core);
        }
    }
    if (physicalCores != nullptr) {
        *physicalCores = static_cast<int>(all.size());
    }
    if (logicalCpus != nullptr) {
        *logicalCpus = static_cast<int>(cpus.size());
    }
    int threads = static_cast<int>(fast.size());
    if (usableCpus > 0) {
        threads = std::min(threads, usableCpus); // pinned (taskset, a container's cpuset)
    }
    return std::max(threads, 1);
}

HardwareInfo HardwareProbe::probe(const QString& root, int usableCpus)
{
    HardwareInfo info;
    const QString base = root.endsWith(u'/') ? root : root + u'/';
    const QString mem = readText(base + u"proc/meminfo"_s);
    info.totalMemory = meminfo(mem, u"MemTotal");
    info.availableMemory = meminfo(mem, u"MemAvailable");
    if (const qint64 limit = cgroupLimit(base); limit > 0 && limit < info.totalMemory) {
        info.totalMemory = limit;
        info.availableMemory = std::min(info.availableMemory, limit);
    }
    const QString cpuinfo = readText(base + u"proc/cpuinfo"_s);
    info.cpuModel = cpuModel(cpuinfo);
    info.cpuFeatures = cpuFeatures(cpuinfo);
    info.usableCpus = usableCpus >= 0 ? usableCpus : affinityCpus();
    info.threads = threadsFor(base, info.usableCpus, &info.physicalCores, &info.logicalCpus);
    return info;
}

} // namespace paraphrase::models
