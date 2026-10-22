#include "ui/core/request_runner.h"

#include <QDBusMessage>
#include <QJsonDocument>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kWarmDelayMs = 800; ///< after the last keystroke (ADR-005)

} // namespace

RequestRunner::RequestRunner(client::DaemonClient& client, QObject* parent)
    : QObject(parent)
    , m_client(client)
{
    m_warmTimer.setSingleShot(true);
    m_warmTimer.setInterval(kWarmDelayMs);
    connect(&m_warmTimer, &QTimer::timeout, this, [this] {
        if (!busy() && !m_warmSkill.isEmpty() && !m_warmInput.isEmpty()) {
            (void)m_client.call(u"Warm"_s, {m_warmSkill, m_warmInput});
        }
    });
    connect(&m_client, &client::DaemonClient::token, this,
            [this](const QString& id, int variant, const QString& text) {
                if (id == m_requestId && variant == 0) {
                    Q_EMIT token(text);
                }
            });
    connect(&m_client, &client::DaemonClient::retrying, this,
            [this](const QString& id, const QString& reason) {
                if (id == m_requestId) {
                    Q_EMIT retrying(reason);
                }
            });
    connect(&m_client, &client::DaemonClient::done, this, [this](const QString& id, const QString& result) {
        if (id == m_requestId) {
            m_requestId.clear();
            Q_EMIT finished(QJsonDocument::fromJson(result.toUtf8()).object());
        }
    });
    connect(&m_client, &client::DaemonClient::error, this,
            [this](const QString& id, const QString& code, const QString& message) {
                if (id == m_requestId) {
                    m_requestId.clear();
                    Q_EMIT failed(code, message);
                }
            });
}

RequestRunner::~RequestRunner()
{
    cancel();
}

QString RequestRunner::start(const QString& skillId, const QVariantMap& input)
{
    m_warmTimer.stop();
    const QDBusMessage reply = m_client.call(u"RunSkill"_s, {skillId, input, QVariantMap()});
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return reply.errorMessage();
    }
    m_requestId = reply.arguments().value(0).toString();
    return {};
}

void RequestRunner::cancel()
{
    if (busy()) {
        (void)m_client.call(u"Cancel"_s, {m_requestId});
    }
}

void RequestRunner::warmLater(const QString& skillId, const QVariantMap& input)
{
    m_warmSkill = skillId;
    m_warmInput = input;
    m_warmTimer.start();
}

} // namespace paraphrase::app
