#include "history/secret_source.h"

#include "history/history_crypto.h"
#include "integration/portal.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QDBusVariant>
#include <QEventLoop>
#include <QTimer>

#include <array>
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

using namespace Qt::StringLiterals;

namespace paraphrase::app::history {
namespace {

constexpr int kSecretBytes = 64;
/// Unlocking a wallet can ask for a password.
constexpr int kPromptTimeoutMs = 120000;
constexpr int kCallTimeoutMs = 5000;
constexpr int kPipeWaitMs = 2000;

// ---- Secret portal ------------------------------------------------------------------------

const QString kPortalSecret = u"org.freedesktop.portal.Secret"_s;

class PortalSecret : public SecretSource
{
public:
    std::optional<QByteArray> fetch() override
    {
        portal::registerApp();
        int fds[2] = {-1, -1};
        if (pipe2(fds, O_CLOEXEC) != 0) {
            return std::nullopt;
        }
        std::optional<QVariantMap> answer;
        {
            // The portal writes the secret, then answers. No copy of the write end may stay
            // here, or reading would never end: QDBusUnixFileDescriptor holds its own.
            const QDBusUnixFileDescriptor writeEnd(fds[1]);
            ::close(fds[1]);
            answer = portal::request(kPortalSecret, u"RetrieveSecret"_s,
                                     {QVariant::fromValue(writeEnd), QVariantMap()}, kPromptTimeoutMs);
        }
        // The portal has written by now, or writes in a moment; never wait longer.
        ::fcntl(fds[0], F_SETFL, O_NONBLOCK);
        if (answer) {
            pollfd ready{.fd = fds[0], .events = POLLIN, .revents = 0};
            (void)::poll(&ready, 1, kPipeWaitMs);
        }
        QByteArray secret;
        while (answer) {
            std::array<char, 256> buf{};
            const ssize_t n = ::read(fds[0], buf.data(), buf.size());
            if (n > 0) {
                secret.append(buf.data(), n);
            } else if (n < 0 && errno == EINTR) {
                continue;
            } else {
                break; // the end, or nothing more yet
            }
        }
        ::close(fds[0]);
        if (secret.isEmpty()) {
            return std::nullopt;
        }
        return secret;
    }
    void forget() override {}
    [[nodiscard]] QString kind() const override { return u"portal"_s; }
};

// ---- Secret Service -----------------------------------------------------------------------

const QString kService = u"org.freedesktop.secrets"_s;
const QString kServicePath = u"/org/freedesktop/secrets"_s;
const QString kServiceIface = u"org.freedesktop.Secret.Service"_s;
const QString kDefaultCollection = u"/org/freedesktop/secrets/aliases/default"_s;

/// The Secret Service's (oayays): session, parameters, value, content type.
struct Secret
{
    QDBusObjectPath session;
    QByteArray parameters;
    QByteArray value;
    QString contentType;
};

QDBusArgument& operator<<(QDBusArgument& arg, const Secret& s)
{
    arg.beginStructure();
    arg << s.session << s.parameters << s.value << s.contentType;
    arg.endStructure();
    return arg;
}

const QDBusArgument& operator>>(const QDBusArgument& arg, Secret& s)
{
    arg.beginStructure();
    arg >> s.session >> s.parameters >> s.value >> s.contentType;
    arg.endStructure();
    return arg; // NOLINT(bugprone-return-const-ref-from-parameter): QtDBus's signature
}

} // namespace
} // namespace paraphrase::app::history

Q_DECLARE_METATYPE(paraphrase::app::history::Secret)

namespace paraphrase::app::history {
namespace {

using SecretMap = QMap<QDBusObjectPath, Secret>;

QMap<QString, QString> attributes()
{
    return {{u"application"_s, u"com.ktechpit.paraphrase"_s}, {u"purpose"_s, u"history"_s}};
}

QDBusMessage call(const QString& path, const QString& iface, const QString& method, const QVariantList& args)
{
    QDBusMessage m = QDBusMessage::createMethodCall(kService, path, iface, method);
    m.setArguments(args);
    return QDBusConnection::sessionBus().call(m, QDBus::Block, kCallTimeoutMs);
}

/// Receives a Prompt's Completed signal.
class PromptWaiter : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(PromptWaiter)

public:
    PromptWaiter() = default;
    ~PromptWaiter() override = default;
    bool done = false;
    bool dismissed = true;

public Q_SLOTS:
    void onCompleted(bool wasDismissed, const QDBusVariant& /*result*/)
    {
        done = true;
        dismissed = wasDismissed;
        Q_EMIT finished();
    }

Q_SIGNALS:
    void finished();
};

/// Runs a prompt the service asked for ("/" means none); true when it was not dismissed.
bool runPrompt(const QDBusObjectPath& prompt)
{
    if (prompt.path() == u"/"_s) {
        return true;
    }
    PromptWaiter waiter;
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.connect(kService, prompt.path(), u"org.freedesktop.Secret.Prompt"_s, u"Completed"_s, &waiter,
                SLOT(onCompleted(bool, QDBusVariant)));
    const QDBusMessage reply =
        call(prompt.path(), u"org.freedesktop.Secret.Prompt"_s, u"Prompt"_s, {QString()});
    if (reply.type() == QDBusMessage::ReplyMessage && !waiter.done) {
        QEventLoop loop;
        QObject::connect(&waiter, &PromptWaiter::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(kPromptTimeoutMs, &loop, &QEventLoop::quit);
        loop.exec();
    }
    bus.disconnect(kService, prompt.path(), u"org.freedesktop.Secret.Prompt"_s, u"Completed"_s, &waiter,
                   SLOT(onCompleted(bool, QDBusVariant)));
    return waiter.done && !waiter.dismissed;
}

bool unlock(const QList<QDBusObjectPath>& objects)
{
    if (objects.isEmpty()) {
        return true;
    }
    const QDBusMessage reply = call(kServicePath, kServiceIface, u"Unlock"_s, {QVariant::fromValue(objects)});
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().size() < 2) {
        return false;
    }
    return runPrompt(qdbus_cast<QDBusObjectPath>(reply.arguments().at(1)));
}

class ServiceSecret : public SecretSource
{
public:
    ServiceSecret()
    {
        qDBusRegisterMetaType<Secret>();
        qDBusRegisterMetaType<SecretMap>();
        qDBusRegisterMetaType<QMap<QString, QString>>();
    }

    std::optional<QByteArray> fetch() override
    {
        // A plain session: the secret crosses only the local bus, as with every keyring client.
        const QDBusMessage opened = call(kServicePath, kServiceIface, u"OpenSession"_s,
                                         {u"plain"_s, QVariant::fromValue(QDBusVariant(QString()))});
        if (opened.type() != QDBusMessage::ReplyMessage || opened.arguments().size() < 2) {
            return std::nullopt;
        }
        const auto session = qdbus_cast<QDBusObjectPath>(opened.arguments().at(1));
        std::optional<QByteArray> out = find(session);
        if (!out) {
            out = create(session);
        }
        (void)call(session.path(), u"org.freedesktop.Secret.Session"_s, u"Close"_s, {});
        return out;
    }

    void forget() override
    {
        for (const QDBusObjectPath& item : items()) {
            const QDBusMessage reply = call(item.path(), u"org.freedesktop.Secret.Item"_s, u"Delete"_s, {});
            if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
                runPrompt(qdbus_cast<QDBusObjectPath>(reply.arguments().at(0)));
            }
        }
    }

    [[nodiscard]] QString kind() const override { return u"service"_s; }

private:
    /// Every item that holds the history's secret, unlocked ones first.
    static QList<QDBusObjectPath> items(QList<QDBusObjectPath>* locked = nullptr)
    {
        const QDBusMessage reply =
            call(kServicePath, kServiceIface, u"SearchItems"_s, {QVariant::fromValue(attributes())});
        if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().size() < 2) {
            return {};
        }
        auto open = qdbus_cast<QList<QDBusObjectPath>>(reply.arguments().at(0));
        const auto shut = qdbus_cast<QList<QDBusObjectPath>>(reply.arguments().at(1));
        if (locked != nullptr) {
            *locked = shut;
        }
        open.append(shut);
        return open;
    }

    static std::optional<QByteArray> find(const QDBusObjectPath& session)
    {
        QList<QDBusObjectPath> locked;
        const QList<QDBusObjectPath> found = items(&locked);
        if (found.isEmpty() || !unlock(locked)) {
            return std::nullopt;
        }
        const QDBusMessage reply =
            call(kServicePath, kServiceIface, u"GetSecrets"_s,
                 {QVariant::fromValue(QList<QDBusObjectPath>{found.first()}), QVariant::fromValue(session)});
        if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
            return std::nullopt;
        }
        const auto secrets = qdbus_cast<SecretMap>(reply.arguments().at(0));
        if (secrets.isEmpty() || secrets.first().value.isEmpty()) {
            return std::nullopt;
        }
        return secrets.first().value;
    }

    static std::optional<QByteArray> create(const QDBusObjectPath& session)
    {
        if (!unlock({QDBusObjectPath(kDefaultCollection)})) {
            return std::nullopt;
        }
        const QByteArray value = randomBytes(kSecretBytes).toBase64();
        if (value.isEmpty()) {
            return std::nullopt;
        }
        const QVariantMap properties{
            {u"org.freedesktop.Secret.Item.Label"_s, u"Paraphrase history"_s},
            {u"org.freedesktop.Secret.Item.Attributes"_s, QVariant::fromValue(attributes())},
        };
        const Secret secret{
            .session = session,
            .parameters = {},
            .value = value,
            .contentType = u"text/plain"_s,
        };
        const QDBusMessage reply = call(kDefaultCollection, u"org.freedesktop.Secret.Collection"_s,
                                        u"CreateItem"_s, {properties, QVariant::fromValue(secret), true});
        if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().size() < 2) {
            return std::nullopt;
        }
        if (!runPrompt(qdbus_cast<QDBusObjectPath>(reply.arguments().at(1)))) {
            return std::nullopt;
        }
        // Read it back: a prompt may have made the item somewhere else.
        return find(session);
    }
};

bool serviceRunning()
{
    const QDBusConnectionInterface* bus = QDBusConnection::sessionBus().interface();
    return bus != nullptr &&
           (bus->isServiceRegistered(kService) || bus->activatableServiceNames().value().contains(kService));
}

} // namespace

std::unique_ptr<SecretSource> desktopSecretSource(const QString& kind)
{
    if ((kind.isEmpty() || kind == u"portal"_s) && portal::hasInterface(kPortalSecret)) {
        return std::make_unique<PortalSecret>();
    }
    if ((kind.isEmpty() || kind == u"service"_s) && serviceRunning()) {
        return std::make_unique<ServiceSecret>();
    }
    return nullptr;
}

} // namespace paraphrase::app::history

#include "secret_source.moc"
