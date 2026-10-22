#include "word_diff.h"

#include <vector>

namespace paraphrase::client {
namespace {

enum class Kind : std::uint8_t
{
    Word,
    Space,
    Other,
};

Kind kindOf(QChar c)
{
    if (c.isLetterOrNumber() || c.isMark() || c == u'\'' || c == u'’') {
        return Kind::Word;
    }
    return c.isSpace() ? Kind::Space : Kind::Other;
}

/// Appends `text` to the last segment when it has the same kind, else starts one.
void append(QList<DiffSegment>& out, const QString& before, const QString& after, bool changed)
{
    if (!out.isEmpty() && out.last().changed == changed) {
        out.last().before += before;
        out.last().after += after;
        return;
    }
    out.append({.before = before, .after = after, .changed = changed});
}

} // namespace

QStringList WordDiff::tokens(const QString& text)
{
    QStringList out;
    qsizetype i = 0;
    while (i < text.size()) {
        const Kind kind = kindOf(text.at(i));
        qsizetype j = i + 1;
        if (kind != Kind::Other) { // punctuation is one mark at a time
            while (j < text.size() && kindOf(text.at(j)) == kind) {
                ++j;
            }
        }
        out.append(text.mid(i, j - i));
        i = j;
    }
    return out;
}

WordDiff::WordDiff(const QString& before, const QString& after)
{
    const QStringList a = tokens(before);
    const QStringList b = tokens(after);
    const auto n = static_cast<size_t>(a.size());
    const auto m = static_cast<size_t>(b.size());
    // Longest common subsequence of tokens; a skill's text is at most a few thousand.
    std::vector<std::vector<int>> lcs(n + 1, std::vector<int>(m + 1, 0));
    for (size_t i = n; i-- > 0;) {
        for (size_t j = m; j-- > 0;) {
            lcs[i][j] = a.at(static_cast<qsizetype>(i)) == b.at(static_cast<qsizetype>(j))
                            ? lcs[i + 1][j + 1] + 1
                            : std::max(lcs[i + 1][j], lcs[i][j + 1]);
        }
    }
    size_t i = 0;
    size_t j = 0;
    while (i < n || j < m) {
        if (i < n && j < m && a.at(static_cast<qsizetype>(i)) == b.at(static_cast<qsizetype>(j))) {
            append(m_segments, a.at(static_cast<qsizetype>(i)), a.at(static_cast<qsizetype>(i)), false);
            ++i;
            ++j;
        } else if (j < m && (i == n || lcs[i][j + 1] >= lcs[i + 1][j])) {
            append(m_segments, {}, b.at(static_cast<qsizetype>(j)), true);
            ++j;
        } else {
            append(m_segments, a.at(static_cast<qsizetype>(i)), {}, true);
            ++i;
        }
    }
}

QList<qsizetype> WordDiff::changes() const
{
    QList<qsizetype> out;
    for (qsizetype k = 0; k < m_segments.size(); ++k) {
        if (m_segments.at(k).changed) {
            out.append(k);
        }
    }
    return out;
}

QString WordDiff::result(const QSet<qsizetype>& undone) const
{
    QString out;
    for (qsizetype k = 0; k < m_segments.size(); ++k) {
        const DiffSegment& s = m_segments.at(k);
        out += s.changed && undone.contains(k) ? s.before : s.after;
    }
    return out;
}

} // namespace paraphrase::client
