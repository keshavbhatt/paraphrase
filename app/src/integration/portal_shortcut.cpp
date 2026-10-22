#include "portal_shortcut.h"

#include "portal.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QKeySequence>

using namespace Qt::StringLiterals;

namespace {

const QString kInterface = u"org.freedesktop.portal.GlobalShortcuts"_s;

/// One entry of BindShortcuts' a(sa{sv}).
struct PortalShortcutEntry
{
    QString id;
    QVariantMap properties;
};

QDBusArgument& operator<<(QDBusArgument& arg, const PortalShortcutEntry& s)
{
    arg.beginStructure();
    arg << s.id << s.properties;
    arg.endStructure();
    return arg;
}

// The signature QtDBus requires for demarshalling.
const QDBusArgument& operator>>(const QDBusArgument& arg, PortalShortcutEntry& s)
{
    arg.beginStructure();
    arg >> s.id >> s.properties;
    arg.endStructure();
    return arg; // NOLINT(bugprone-return-const-ref-from-parameter): QtDBus's signature
}

} // namespace

Q_DECLARE_METATYPE(PortalShortcutEntry)

namespace paraphrase::app {

PortalShortcut::PortalShortcut(QObject* parent)
    : ShortcutBackend(parent)
    , m_available(portal::hasInterface(kInterface))
{
    qDBusRegisterMetaType<PortalShortcutEntry>();
    qDBusRegisterMetaType<QList<PortalShortcutEntry>>();
}

PortalShortcut::~PortalShortcut()
{
    close();
}

QString PortalShortcut::name() const
{
    return tr("Desktop shortcut (GlobalShortcuts portal)");
}

QString PortalShortcut::preferredTrigger(const QKeySequence& keys)
{
    if (keys.isEmpty()) {
        return {};
    }
    const QKeyCombination combo = keys[0];
    QStringList parts;
    const Qt::KeyboardModifiers mods = combo.keyboardModifiers();
    if (mods.testFlag(Qt::ControlModifier)) {
        parts << u"CTRL"_s;
    }
    if (mods.testFlag(Qt::AltModifier)) {
        parts << u"ALT"_s;
    }
    if (mods.testFlag(Qt::ShiftModifier)) {
        parts << u"SHIFT"_s;
    }
    if (mods.testFlag(Qt::MetaModifier)) {
        parts << u"LOGO"_s;
    }
    // Key names as xkb keysym names: "space", "a", "F2".
    const QString key = QKeySequence(combo.key()).toString(QKeySequence::PortableText);
    if (combo.key() == Qt::Key_Space) {
        parts << u"space"_s;
    } else {
        parts << (key.size() == 1 ? key.toLower() : key);
    }
    return parts.join(u'+');
}

bool PortalShortcut::createSession()
{
    portal::registerApp();
    const auto created = portal::request(kInterface, u"CreateSession"_s,
                                         {QVariantMap{{u"session_handle_token"_s, portal::token()}}}, 10'000);
    if (!created) {
        return false;
    }
    m_session = created->value(u"session_handle"_s).toString();
    QDBusConnection::sessionBus().connect(
        QString::fromLatin1(portal::kService), QString::fromLatin1(portal::kPath), kInterface, u"Activated"_s,
        this, SLOT(onActivated(QDBusObjectPath, QString, qulonglong, QVariantMap)));
    return !m_session.isEmpty();
}

bool PortalShortcut::bind(const QKeySequence& keys)
{
    if (!m_available || (m_session.isEmpty() && !createSession())) {
        return false;
    }
    const QList<PortalShortcutEntry> shortcuts{
        {
            .id = QString::fromLatin1(kShortcutId),
            .properties =
                {
                    {u"description"_s, tr("Open the Paraphrase popup")},
                    {u"preferred_trigger"_s, preferredTrigger(keys)},
                },
        },
    };
    const auto bound = portal::request(kInterface, u"BindShortcuts"_s,
                                       {
                                           QVariant::fromValue(QDBusObjectPath(m_session)),
                                           QVariant::fromValue(shortcuts),
                                           QString(),
                                           QVariantMap(),
                                       },
                                       kBindTimeoutMs);
    if (!bound) {
        return false;
    }
    readTrigger(*bound);
    return true;
}

void PortalShortcut::readTrigger(const QVariantMap& results)
{
    // The desktop says which keys the user ended up with ("trigger_description").
    const auto arg = results.value(u"shortcuts"_s).value<QDBusArgument>();
    QList<PortalShortcutEntry> shortcuts;
    arg >> shortcuts;
    for (const PortalShortcutEntry& s : std::as_const(shortcuts)) {
        if (s.id == QLatin1StringView(kShortcutId)) {
            m_trigger = s.properties.value(u"trigger_description"_s).toString();
        }
    }
}

QString PortalShortcut::description() const
{
    if (!m_trigger.isEmpty()) {
        return tr("%1 opens the popup; change it in your desktop's shortcut settings").arg(m_trigger);
    }
    return tr("Set in your desktop's shortcut settings, under Paraphrase");
}

void PortalShortcut::onActivated(const QDBusObjectPath& session, const QString& id, qulonglong /*timestamp*/,
                                 const QVariantMap& /*options*/)
{
    if (session.path() == m_session && id == QLatin1StringView(kShortcutId)) {
        Q_EMIT activated();
    }
}

void PortalShortcut::close()
{
    if (m_session.isEmpty()) {
        return;
    }
    const QDBusMessage call = QDBusMessage::createMethodCall(QString::fromLatin1(portal::kService), m_session,
                                                             u"org.freedesktop.portal.Session"_s, u"Close"_s);
    QDBusConnection::sessionBus().call(call, QDBus::NoBlock);
    m_session.clear();
}

} // namespace paraphrase::app
