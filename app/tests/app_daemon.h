#pragma once

// A real daemon service with the fake engine, on a private socket, for the app's window tests.
// Models download from a local test server (tiny files), so a download finishes in a moment.

#include "fake_daemon.h"
#include "test_http_server.h"

#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

namespace paraphrase::testing {

class AppDaemon
{
public:
    static constexpr int kTimeoutMs = 10'000;

    /// Starts the daemon with `installed` models on a computer with `memoryGb`, and points
    /// clients at it through PARAPHRASE_DAEMON_ADDRESS.
    bool start(const QStringList& installed, double memoryGb = 32)
    {
        using namespace Qt::StringLiterals;
        stop();
        m_dir = std::make_unique<QTemporaryDir>();
        state = std::make_shared<FakeState>();
        const auto catalog = paraphrase::models::ModelCatalog::parse(tinyCatalog()).value();
        if (!install(catalog, m_dir->path() + u"/models"_s, installed)) {
            return false;
        }
        auto skills = paraphrase::skills::SkillRegistry::load(QStringLiteral(PARAPHRASE_REPO_ROOT) + u"/skills"_s);
        if (!skills.ok()) {
            return false;
        }
        paraphrase::models::HardwareInfo hw;
        hw.totalMemory = static_cast<qint64>(memoryGb * 1024 * 1024 * 1024);
        hw.threads = 4;
        hw.logicalCpus = 8;
        hw.cpuModel = u"Test CPU"_s;
        m_http = std::make_unique<TestHttpServer>();
        for (const auto& m : catalog.models) {
            m_http->addRoute(u'/' + m.files.front().file, {.body = m.id.toUtf8(), .supportsRange = true});
        }
        const QUrl base = m_http->urlFor(u"/"_s);
        const QString address = u"unix:path="_s + m_dir->path() + u"/daemon.sock"_s;
        m_server = std::make_unique<ServerThread>(
            ServiceSetup{
                .skills = std::move(skills).value(),
                .catalog = catalog,
                .modelsRoot = m_dir->path() + u"/models"_s,
                .hardware = hw,
                .engineFactory = [s = state] { return std::make_unique<FakeEngine>(s); },
                .urlResolver =
                    [base](const paraphrase::models::ModelEntry&, const paraphrase::models::ModelFile& f) {
                        return base.resolved(QUrl(f.file));
                    },
                .idleUnloadMs = 60'000,
                .idleExitMs = 60'000,
                .settingsPath = m_dir->path() + u"/config/daemon.json"_s,
            },
            address);
        m_server->start();
        if (!QTest::qWaitFor([this] { return m_server->ready.load(); }, kTimeoutMs)) {
            return false;
        }
        qputenv("PARAPHRASE_DAEMON_ADDRESS", address.toUtf8());
        return true;
    }

    void stop()
    {
        qunsetenv("PARAPHRASE_DAEMON_ADDRESS");
        if (m_server) {
            m_server->quit();
            m_server->wait();
            m_server.reset();
        }
        m_http.reset();
    }

    ~AppDaemon() { stop(); }

    std::shared_ptr<FakeState> state;

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<TestHttpServer> m_http;
    std::unique_ptr<ServerThread> m_server;
};

} // namespace paraphrase::testing
