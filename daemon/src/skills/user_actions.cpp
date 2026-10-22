#include "user_actions.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::skills {
namespace {

QJsonObject toJson(const UserAction& a)
{
    return {{u"id"_s, a.id}, {u"name"_s, a.name}, {u"instruction"_s, a.instruction}, {u"tier"_s, a.tier}};
}

UserAction fromJson(const QJsonObject& o)
{
    return {
        .id = o.value(u"id"_s).toString(),
        .name = o.value(u"name"_s).toString(),
        .instruction = o.value(u"instruction"_s).toString(),
        .tier = o.value(u"tier"_s).toString(),
    };
}

QString slug(const QString& name)
{
    static const QRegularExpression kNotWord(u"[^a-z0-9]+"_s);
    QString s = name.toLower().replace(kNotWord, u"-"_s);
    while (s.startsWith(u'-')) {
        s.remove(0, 1);
    }
    while (s.endsWith(u'-')) {
        s.chop(1);
    }
    return s.isEmpty() ? u"action"_s : s.left(32);
}

} // namespace

util::Result<bool> UserActions::load()
{
    using R = util::Result<bool>;
    m_actions.clear();
    QFile file(m_path);
    if (!file.exists()) {
        return R::success(true);
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return R::failure(u"cannot read %1"_s.arg(QFileInfo(m_path).fileName()));
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        return R::failure(u"%1 is not a list of actions"_s.arg(QFileInfo(m_path).fileName()));
    }
    for (const auto& v : doc.array()) {
        const UserAction a = fromJson(v.toObject());
        if (check(a).isEmpty() && !a.id.isEmpty() && m_actions.size() < kMaxActions) {
            m_actions.append(a);
        }
    }
    return R::success(true);
}

QString UserActions::check(const UserAction& action)
{
    if (action.name.trimmed().isEmpty() || action.name.size() > kMaxName) {
        return QObject::tr("Give the action a name of at most %1 characters.").arg(kMaxName);
    }
    if (action.instruction.trimmed().isEmpty() || action.instruction.size() > kMaxInstruction) {
        return QObject::tr("Say what the action does, in at most %1 characters.").arg(kMaxInstruction);
    }
    if (action.tier != u"fast"_s && action.tier != u"quality"_s) {
        return QObject::tr("The model must be fast or quality.");
    }
    return {};
}

QString UserActions::uniqueId(const QString& name) const
{
    const QString base = slug(name);
    QString id = base;
    for (int n = 2; std::ranges::any_of(m_actions, [&](const UserAction& a) { return a.id == id; }); ++n) {
        id = u"%1-%2"_s.arg(base).arg(n);
    }
    return id;
}

util::Result<QString> UserActions::save(const UserAction& action)
{
    using R = util::Result<QString>;
    if (const QString problem = check(action); !problem.isEmpty()) {
        return R::failure(problem);
    }
    UserAction a = action;
    a.name = a.name.trimmed();
    a.instruction = a.instruction.trimmed();
    const auto it = std::ranges::find(m_actions, a.id, &UserAction::id);
    if (a.id.isEmpty() || it == m_actions.end()) {
        if (m_actions.size() >= kMaxActions) {
            return R::failure(QObject::tr("You have %1 actions already: delete one first.").arg(kMaxActions));
        }
        a.id = uniqueId(a.name);
        m_actions.append(a);
    } else {
        *it = a;
    }
    if (const auto written = write(); !written.ok()) {
        return R::failure(written.error());
    }
    return R::success(a.id);
}

util::Result<bool> UserActions::remove(const QString& id)
{
    const auto removed = m_actions.removeIf([&](const UserAction& a) { return a.id == id; });
    if (removed == 0) {
        return util::Result<bool>::failure(QObject::tr("There is no action \"%1\".").arg(id));
    }
    return write();
}

util::Result<bool> UserActions::write() const
{
    using R = util::Result<bool>;
    if (m_path.isEmpty()) {
        return R::success(true); // kept in memory only (tests)
    }
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QJsonArray list;
    for (const UserAction& a : m_actions) {
        list.append(toJson(a));
    }
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(list).toJson()) < 0 || !file.commit()) {
        return R::failure(QObject::tr("Cannot save your actions."));
    }
    return R::success(true);
}

} // namespace paraphrase::skills
