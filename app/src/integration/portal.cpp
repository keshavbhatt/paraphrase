#include "portal.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>
#include <QEventLoop>
#include <QTimer>

#include <algorithm>
#include <atomic>

using namespace Qt::StringLiterals;

namespace paraphrase::app::portal {
namespace {

const QString kAppId = u"com.ktechpit.paraphrase"_s;

/// The Request object the portal will create for this token (xdg-desktop-portal's rule):
/// /org/freedesktop/portal/desktop/request/<sender without ':' and with '.' as '_'>/<token>.
QString requestPath(const QString& handleToken)
{
    QString sender = QDBusConnection::sessionBus().baseService().mid(1);
    sender.replace(u'.', u'_');
    return u"/org/freedesktop/portal/desktop/request/%1/%2"_s.arg(sender, handleToken);
}

} // namespace

void ResponseCatcher::onResponse(uint response, const QVariantMap& answer)
{
    code = response;
    results = answer;
    Q_EMIT received();
}

bool hasInterface(const QString& interface)
{
    return version(interface) > 0;
}

uint version(const QString& interface)
{
    QDBusMessage get =
        QDBusMessage::createMethodCall(QString::fromLatin1(kService), QString::fromLatin1(kPath),
                                       u"org.freedesktop.DBus.Properties"_s, u"Get"_s);
    get << interface << u"version"_s;
    const QDBusMessage reply = QDBusConnection::sessionBus().call(get, QDBus::Block, 2000);
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return 0;
    }
    // Every portal interface starts at version 1; an answer without a number still means one.
    return std::max(1U, reply.arguments().value(0).value<QDBusVariant>().variant().toUInt());
}

void registerApp()
{
    static std::atomic<bool> done{false};
    if (done.exchange(true)) {
        return;
    }
    QDBusMessage call =
        QDBusMessage::createMethodCall(QString::fromLatin1(kService), QString::fromLatin1(kPath),
                                       u"org.freedesktop.host.portal.Registry"_s, u"Register"_s);
    call << kAppId << QVariantMap();
    // An older portal without the registry, or a sandboxed app: nothing to do.
    (void)QDBusConnection::sessionBus().call(call, QDBus::Block, 2000);
}

QString token()
{
    static std::atomic<int> counter{0};
    return u"paraphrase%1"_s.arg(++counter);
}

std::optional<QVariantMap> request(const QString& interface, const QString& method, QVariantList args,
                                   int timeoutMs, int* response)
{
    if (response != nullptr) {
        *response = kNoResponse;
    }
    if (args.isEmpty() || args.back().typeId() != QMetaType::QVariantMap) {
        return std::nullopt;
    }
    const QString handle = token();
    QVariantMap options = args.back().toMap();
    options.insert(u"handle_token"_s, handle);
    args.back() = options;

    // Listen before calling, so a quick answer is not missed.
    ResponseCatcher catcher;
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.connect(QString::fromLatin1(kService), requestPath(handle), u"org.freedesktop.portal.Request"_s,
                u"Response"_s, &catcher, SLOT(onResponse(uint, QVariantMap)));
    QDBusMessage call = QDBusMessage::createMethodCall(QString::fromLatin1(kService),
                                                       QString::fromLatin1(kPath), interface, method);
    call.setArguments(args);
    const QDBusMessage reply = bus.call(call, QDBus::Block, timeoutMs);
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return std::nullopt;
    }
    if (!catcher.code) {
        QEventLoop loop;
        QObject::connect(&catcher, &ResponseCatcher::received, &loop, &QEventLoop::quit);
        QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
        loop.exec();
    }
    bus.disconnect(QString::fromLatin1(kService), requestPath(handle), u"org.freedesktop.portal.Request"_s,
                   u"Response"_s, &catcher, SLOT(onResponse(uint, QVariantMap)));
    if (response != nullptr && catcher.code) {
        *response = static_cast<int>(*catcher.code);
    }
    if (!catcher.code || *catcher.code != 0) {
        return std::nullopt; // 1: the user cancelled, 2: another error
    }
    return catcher.results;
}

} // namespace paraphrase::app::portal
