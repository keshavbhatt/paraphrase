#pragma once

#include <QString>
#include <QStringList>

namespace paraphrase::models {

/// What the computer offers a model (PLAN D9): memory, CPU cores and features. GPUs come
/// from the engine (llama.cpp's device list), which is the only code that may ask it.
struct HardwareInfo
{
    qint64 totalMemory = 0;     ///< bytes the daemon may use: RAM, or a lower cgroup limit
    qint64 availableMemory = 0; ///< bytes free now (MemAvailable)
    int logicalCpus = 0;
    int physicalCores = 0;
    int usableCpus = 0; ///< CPUs this process may run on (affinity, cgroup cpusets)
    int threads = 1;    ///< what the engine should use, see HardwareProbe
    QString cpuModel;
    QStringList cpuFeatures; ///< the ones that matter to llama.cpp: avx2, avx512f, ...
    QStringList gpus;        ///< filled in by the engine

    [[nodiscard]] double totalMemoryGb() const;
};

/// Reads `/proc` and `/sys` (from `root`, so tests can give it a fake tree). Pure reading,
/// nothing is run or timed.
///
/// Threads (ADR-016): one per physical core, leaving out low-power cores. On Intel hybrid
/// CPUs those are the cores without the shared L3 cache (the Core Ultra 255U's two LP E-cores);
/// the M0 sweep found 10 threads fastest on it, its 2 P-cores plus 8 E-cores. Hyper-threads
/// do not help token generation, and llama.cpp's own default (P-cores only) is 2 there.
class HardwareProbe
{
public:
    /// `usableCpus` < 0 asks the kernel (sched_getaffinity); tests pass a number.
    [[nodiscard]] static HardwareInfo probe(const QString& root = QStringLiteral("/"), int usableCpus = -1);

    /// Threads for llama.cpp from the CPU topology under `root`.
    [[nodiscard]] static int threadsFor(const QString& root, int usableCpus, int* physicalCores = nullptr,
                                        int* logicalCpus = nullptr);
};

} // namespace paraphrase::models
