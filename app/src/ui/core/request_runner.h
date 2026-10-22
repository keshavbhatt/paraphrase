#pragma once

#include "daemon_client.h"

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantMap>

namespace paraphrase::app {

/// One request at a time to the daemon, as the window needs it: start, stream, finish or
/// fail, cancel; and reading ahead (Warm) a moment after the user stops typing (ADR-005).
/// Events of other requests are ignored, so a stopped request can never overwrite a result.
class RequestRunner : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(RequestRunner)

public:
    explicit RequestRunner(client::DaemonClient& client, QObject* parent = nullptr);
    ~RequestRunner() override;

    /// Sends the request; "" when it runs, else the daemon's message.
    [[nodiscard]] QString start(const QString& skillId, const QVariantMap& input);
    void cancel();
    [[nodiscard]] bool busy() const { return !m_requestId.isEmpty(); }

    /// Warm the model for this request after a short pause; a new call restarts the pause.
    void warmLater(const QString& skillId, const QVariantMap& input);

Q_SIGNALS:
    void token(const QString& piece);
    void retrying(const QString& reason);
    void finished(const QJsonObject& result);
    void failed(const QString& code, const QString& message);

private:
    client::DaemonClient& m_client;
    QString m_requestId;
    QTimer m_warmTimer;
    QString m_warmSkill;
    QVariantMap m_warmInput;
};

} // namespace paraphrase::app
