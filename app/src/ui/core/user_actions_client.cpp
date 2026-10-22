#include "ui/core/user_actions_client.h"

#include "daemon_client.h"

#include <QDBusMessage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

bool UserActionsClient::load()
{
    m_actions.clear();
    const QDBusMessage reply = m_client.call(u"ListActions"_s, {});
    const QJsonObject o = QJsonDocument::fromJson(reply.arguments().value(0).toString().toUtf8()).object();
    m_supported = reply.type() == QDBusMessage::ReplyMessage && o.value(u"supported"_s).toBool();
    m_max = o.value(u"max"_s).toInt();
    m_template = o.value(u"template"_s).toString();
    for (const auto& v : o.value(u"actions"_s).toArray()) {
        const QJsonObject a = v.toObject();
        m_actions.append({
            .id = a.value(u"id"_s).toString(),
            .skill = a.value(u"skill"_s).toString(),
            .name = a.value(u"name"_s).toString(),
            .instruction = a.value(u"instruction"_s).toString(),
            .tier = a.value(u"tier"_s).toString(),
        });
    }
    return m_supported;
}

QString UserActionsClient::save(const ActionInfo& action, QString* skillId)
{
    const QDBusMessage reply =
        m_client.call(u"SaveAction"_s, {action.id, action.name, action.instruction, action.tier});
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return reply.errorMessage();
    }
    if (skillId != nullptr) {
        *skillId = reply.arguments().value(0).toString();
    }
    (void)load();
    return {};
}

QString UserActionsClient::remove(const QString& id)
{
    const QDBusMessage reply = m_client.call(u"DeleteAction"_s, {id});
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return reply.errorMessage();
    }
    (void)load();
    return {};
}

} // namespace paraphrase::app
