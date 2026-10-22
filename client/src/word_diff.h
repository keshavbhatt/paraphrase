#pragma once

#include <QList>
#include <QSet>
#include <QString>

namespace paraphrase::client {

/// One piece of a word diff: text both versions share, or one change (what was there before
/// and what is there after; either may be empty).
struct DiffSegment
{
    QString before;
    QString after;
    bool changed = false;

    [[nodiscard]] bool operator==(const DiffSegment&) const = default;
};

/// The word-level diff a client shows for proofread (PLAN D6): the model returns the whole
/// corrected text and the client works out what changed, so every change can be shown and
/// undone one at a time. Words, single punctuation marks and runs of spaces are the units;
/// combining marks stay with their letter (Devanagari matras), so a script is never split
/// mid-character. Adjacent removals and additions form one change ("egg's" to "eggs").
class WordDiff
{
public:
    WordDiff(const QString& before, const QString& after);

    [[nodiscard]] const QList<DiffSegment>& segments() const { return m_segments; }
    /// Indexes into segments() of the changes, in order.
    [[nodiscard]] QList<qsizetype> changes() const;
    /// The corrected text with the changes in `undone` put back as they were.
    [[nodiscard]] QString result(const QSet<qsizetype>& undone = {}) const;

    /// Words, punctuation and spaces, in order; joined they give `text` back exactly.
    [[nodiscard]] static QStringList tokens(const QString& text);

private:
    QList<DiffSegment> m_segments;
};

} // namespace paraphrase::client
