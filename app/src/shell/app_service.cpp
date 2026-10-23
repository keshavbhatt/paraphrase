#include "shell/app_service.h"

#include <QDBusConnection>
#include <QDBusMessage>

namespace paraphrase::app {

bool AppService::registerOnBus()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected() || !bus.registerService(QString::fromLatin1(kService))) {
        return false;
    }
    return bus.registerObject(QString::fromLatin1(kPath), this, QDBusConnection::ExportScriptableSlots);
}

bool AppService::forward(const QString& method, const QString& activationToken)
{
    QDBusMessage call = QDBusMessage::createMethodCall(
        QString::fromLatin1(kService), QString::fromLatin1(kPath), QString::fromLatin1(kService), method);
    call << activationToken;
    return QDBusConnection::sessionBus().call(call).type() == QDBusMessage::ReplyMessage;
}

void AppService::useActivationToken(const QString& token)
{
    if (!token.isEmpty()) {
        qputenv("XDG_ACTIVATION_TOKEN", token.toUtf8());
    }
}

} // namespace paraphrase::app
