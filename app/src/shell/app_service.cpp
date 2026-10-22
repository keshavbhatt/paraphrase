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

bool AppService::forward(const QString& method)
{
    const QDBusMessage call = QDBusMessage::createMethodCall(
        QString::fromLatin1(kService), QString::fromLatin1(kPath), QString::fromLatin1(kService), method);
    return QDBusConnection::sessionBus().call(call).type() == QDBusMessage::ReplyMessage;
}

} // namespace paraphrase::app
