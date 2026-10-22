#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace paraphrase::client {
class DaemonClient;
}

namespace paraphrase::app {

/// Runs one skill on one text through the daemon and reports the answer: the setup window's
/// "Try it" step.
class SampleRun : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(SampleRun)

public:
    explicit SampleRun(client::DaemonClient& client, QObject* parent = nullptr);
    ~SampleRun() override = default;

    /// Starts the run; false when one is running already or the daemon refused it (failed()).
    bool run(const QString& skillId, const QString& text);
    /// The same with any inputs and options (DOCS/api.md, RunSkill).
    bool run(const QString& skillId, const QVariantMap& input, const QVariantMap& options);
    [[nodiscard]] bool running() const { return !m_requestId.isEmpty(); }

Q_SIGNALS:
    void finished(const QString& output);
    void failed(const QString& message);

private:
    client::DaemonClient& m_client;
    QString m_requestId;
};

} // namespace paraphrase::app
