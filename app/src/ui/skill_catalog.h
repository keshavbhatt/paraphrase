#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace paraphrase::app {

/// A skill as the daemon lists it (ListSkills, DOCS/api.md).
struct SkillInfo
{
    QString id;
    QString base;
    QString title;
    QString output;     ///< text | json
    QStringList inputs; ///< the request inputs its prompt uses

    [[nodiscard]] bool takes(const QString& input) const { return inputs.contains(input); }
};

/// What the window shows for a skill: the input and the output it needs.
enum class SkillKind : std::uint8_t
{
    Text,       ///< text in, text out: proofread, tones, summarize, format, translate
    QuickReply, ///< a message in, three short replies out (reply)
    FullReply,  ///< a message and what to say in, one full reply out (draft.*)
};

/// The daemon's skills and what the window derives from them: the actions in their order,
/// presets, titles and kinds. No widgets: the action bar and the window read it.
class SkillCatalog
{
public:
    SkillCatalog() = default;
    [[nodiscard]] static SkillCatalog fromJson(const QString& listSkillsJson);

    [[nodiscard]] const QList<SkillInfo>& skills() const { return m_skills; }
    [[nodiscard]] bool isEmpty() const { return m_skills.isEmpty(); }
    [[nodiscard]] const SkillInfo* find(const QString& id) const;
    [[nodiscard]] bool hasBase(const QString& base) const;
    [[nodiscard]] SkillKind kindOf(const QString& id) const;

    /// The bases shown as actions, in the mocks' order; full replies live inside Reply.
    [[nodiscard]] QStringList actionBases() const;
    [[nodiscard]] QList<const SkillInfo*> variants(const QString& base) const;
    /// A base with one skill that writes in a chosen language (Translate) gets a language menu.
    [[nodiscard]] bool picksLanguage(const QString& base) const;
    /// The full reply skill for an email or a chat message, or "" without one.
    [[nodiscard]] QString fullReplyId(bool chat) const;
    /// The Tone action's presets as (tone, title): what a full reply can be written in.
    [[nodiscard]] QList<std::pair<QString, QString>> tones() const;

    /// "Tone" for rewrite, "Reply" for reply, else the skill's title before a preset's ":".
    [[nodiscard]] QString actionTitle(const QString& base) const;
    /// A preset's own name: "Polite" for "Change tone: Polite".
    [[nodiscard]] static QString presetTitle(const SkillInfo& s);

private:
    QList<SkillInfo> m_skills;
};

} // namespace paraphrase::app
