#pragma once

// A real daemon service with the fake engine on a private socket, for the app's tests: the
// shipped skills, a tiny catalog with the given models installed, and a way to script answers.

#include "fake_daemon.h"

#include <QMutexLocker>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

namespace paraphrase::testing {

class TestDaemon
{
public:
    static constexpr int kTimeoutMs = 10'000;

    TestDaemon() = default;
    ~TestDaemon() { stop(); }
    TestDaemon(const TestDaemon&) = delete;
    TestDaemon& operator=(const TestDaemon&) = delete;
    TestDaemon(TestDaemon&&) = delete;
    TestDaemon& operator=(TestDaemon&&) = delete;

    /// Starts the service and points clients at it (PARAPHRASE_DAEMON_ADDRESS).
    bool start(const QStringList& installed)
    {
        stop();
        m_dir = std::make_unique<QTemporaryDir>();
        m_state = std::make_shared<FakeState>();
        const auto catalog = paraphrase::models::ModelCatalog::parse(tinyCatalog()).value();
        if (!install(catalog, m_dir->path() + QStringLiteral("/models"), installed)) {
            return false;
        }
        auto skills = paraphrase::skills::SkillRegistry::load(QStringLiteral(PARAPHRASE_REPO_ROOT "/skills"));
        if (!skills.ok()) {
            return false;
        }
        paraphrase::models::HardwareInfo hw;
        hw.totalMemory = qint64{32} * 1024 * 1024 * 1024;
        hw.threads = 4;
        const QString address = QStringLiteral("unix:path=") + m_dir->path() + QStringLiteral("/daemon.sock");
        m_server = std::make_unique<ServerThread>(
            ServiceSetup{
                .skills = std::move(skills).value(),
                .catalog = catalog,
                .modelsRoot = m_dir->path() + QStringLiteral("/models"),
                .hardware = hw,
                .engineFactory = [state = m_state] { return std::make_unique<FakeEngine>(state); },
                .urlResolver = {},
                .idleUnloadMs = 60'000,
                .idleExitMs = 60'000,
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
    }

    void setAnswers(const QStringList& answers)
    {
        const QMutexLocker lock(&m_state->lock);
        m_state->answers = answers;
    }

    [[nodiscard]] FakeState& state() const { return *m_state; }

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::shared_ptr<FakeState> m_state;
    std::unique_ptr<ServerThread> m_server;
};

} // namespace paraphrase::testing
