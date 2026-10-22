#include "ui/setup/sample_run.h"

#include "daemon_client.h"

#include <QDBusMessage>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

SampleRun::SampleRun(client::DaemonClient& client, QObject* parent)
    : QObject(parent)
    , m_client(client)
{
    connect(&m_client, &client::DaemonClient::done, this, [this](const QString& id, const QString& result) {
        if (id == m_requestId) {
            m_requestId.clear();
            Q_EMIT finished(
                QJsonDocument::fromJson(result.toUtf8()).object().value(u"output"_s).toString().trimmed());
        }
    });
    connect(&m_client, &client::DaemonClient::error, this,
            [this](const QString& id, const QString&, const QString& message) {
                if (id == m_requestId) {
                    m_requestId.clear();
                    Q_EMIT failed(message);
                }
            });
}

bool SampleRun::run(const QString& skillId, const QString& text)
{
    if (running() || text.trimmed().isEmpty()) {
        return false;
    }
    const QDBusMessage reply =
        m_client.call(u"RunSkill"_s, {skillId, QVariantMap{{u"text"_s, text}}, QVariantMap()});
    if (reply.type() == QDBusMessage::ErrorMessage) {
        Q_EMIT failed(reply.errorMessage());
        return false;
    }
    m_requestId = reply.arguments().value(0).toString();
    return true;
}

} // namespace paraphrase::app
