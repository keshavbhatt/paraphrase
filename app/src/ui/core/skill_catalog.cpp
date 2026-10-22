#include "ui/core/skill_catalog.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

/// The order the actions appear in (main-window.html): the mocks' skills first, then the rest.
int rank(const QString& base)
{
    static const QStringList kOrder = {
        u"proofread"_s, u"rewrite"_s, u"reply"_s, u"summarize"_s, u"format"_s, u"translate"_s,
    };
    const qsizetype i = kOrder.indexOf(base);
    return static_cast<int>(i < 0 ? kOrder.size() : i);
}

const QString kFullReplyBase = u"draft"_s;

} // namespace

SkillCatalog SkillCatalog::fromJson(const QString& listSkillsJson)
{
    SkillCatalog catalog;
    for (const auto& v : QJsonDocument::fromJson(listSkillsJson.toUtf8()).array()) {
        const QJsonObject o = v.toObject();
        QStringList inputs;
        for (const auto& i : o.value(u"inputs"_s).toArray()) {
            inputs.append(i.toString());
        }
        catalog.m_skills.append({
            .id = o.value(u"id"_s).toString(),
            .base = o.value(u"base"_s).toString(),
            .title = o.value(u"title"_s).toString(),
            .output = o.value(u"output"_s).toString(),
            .inputs = inputs,
        });
    }
    std::ranges::stable_sort(
        catalog.m_skills, [](const SkillInfo& a, const SkillInfo& b) { return rank(a.base) < rank(b.base); });
    return catalog;
}

const SkillInfo* SkillCatalog::find(const QString& id) const
{
    const auto it = std::ranges::find(m_skills, id, &SkillInfo::id);
    return it == m_skills.end() ? nullptr : &*it;
}

bool SkillCatalog::hasBase(const QString& base) const
{
    return std::ranges::any_of(m_skills, [&](const SkillInfo& s) { return s.base == base; });
}

SkillKind SkillCatalog::kindOf(const QString& id) const
{
    const SkillInfo* s = find(id);
    if (s == nullptr) {
        return SkillKind::Text;
    }
    if (s->base == kFullReplyBase) {
        return SkillKind::FullReply;
    }
    return s->base == u"reply"_s ? SkillKind::QuickReply : SkillKind::Text;
}

QStringList SkillCatalog::actionBases() const
{
    QStringList bases;
    for (const SkillInfo& s : m_skills) {
        if (s.base != kFullReplyBase && !bases.contains(s.base)) {
            bases.append(s.base);
        }
    }
    if (!m_userActionsBase.isEmpty() && !bases.contains(m_userActionsBase)) {
        bases.append(m_userActionsBase);
    }
    return bases;
}

QList<const SkillInfo*> SkillCatalog::variants(const QString& base) const
{
    QList<const SkillInfo*> out;
    for (const SkillInfo& s : m_skills) {
        if (s.base == base) {
            out.append(&s);
        }
    }
    return out;
}

bool SkillCatalog::picksLanguage(const QString& base) const
{
    const auto v = variants(base);
    return v.size() == 1 && v.front()->takes(u"target_lang"_s);
}

QString SkillCatalog::fullReplyId(bool chat) const
{
    QString wanted = kFullReplyBase + (chat ? u".chat"_s : u".email"_s);
    if (find(wanted) != nullptr) {
        return wanted;
    }
    const auto v = variants(kFullReplyBase);
    return v.isEmpty() ? QString() : v.front()->id;
}

QList<std::pair<QString, QString>> SkillCatalog::tones() const
{
    QList<std::pair<QString, QString>> out;
    for (const SkillInfo* s : variants(u"rewrite"_s)) {
        out.append({s->id.section(u'.', 1), presetTitle(*s)});
    }
    return out;
}

QString SkillCatalog::actionTitle(const QString& base) const
{
    if (base == u"rewrite"_s) {
        return QObject::tr("Tone");
    }
    if (base == u"reply"_s) {
        return QObject::tr("Reply");
    }
    if (base == m_userActionsBase) {
        return QObject::tr("My actions");
    }
    const auto v = variants(base);
    return v.isEmpty() ? base : v.front()->title.section(u':', 0, 0).trimmed();
}

QString SkillCatalog::presetTitle(const SkillInfo& s)
{
    const QString preset = s.title.section(u':', 1).trimmed();
    return preset.isEmpty() ? s.title : preset;
}

} // namespace paraphrase::app
