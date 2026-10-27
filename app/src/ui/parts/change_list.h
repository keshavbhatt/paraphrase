#pragma once

#include "ui/common/theme.h"
#include "word_diff.h"

#include <QScrollArea>
#include <QSet>

class QHBoxLayout;
class QVBoxLayout;

namespace paraphrase::app {

/// Why a word changed, in a few words (popup-proofread.html, note 3): "capital letter",
/// "comma", "spelling", "wording", "added word". Worked out from the two words alone.
[[nodiscard]] QString changeReason(const QString& before, const QString& after);
/// Whitespace as the change list names it: "2 line breaks", "a line break", "a space".
[[nodiscard]] QString spacingName(const QString& space);

/// Proofread's changes, one row each (main-window.html, note 5): the old word, an arrow, the
/// new word, and Undo. An addition shows where it went ("end" of a line) and a removal says
/// so in words: colour is never the only signal.
class ChangeList : public QScrollArea
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ChangeList)

public:
    explicit ChangeList(QWidget* parent = nullptr);
    ~ChangeList() override = default;

    /// Rebuilds the rows; nullptr hides the list.
    void setDiff(const client::WordDiff* diff, const QSet<qsizetype>& undone);
    void applyTheme(const Tokens& t) { m_tokens = t; }
    /// In a split the list may be made taller or shorter than its rows need; fitHeight() is
    /// still what they need, up to a cap.
    void setResizable(bool resizable) { m_resizable = resizable; }
    [[nodiscard]] int fitHeight() const { return m_fitHeight; }

Q_SIGNALS:
    void undoToggled(qsizetype change);

private:
    [[nodiscard]] QWidget* row(const client::WordDiff& diff, qsizetype k, bool undone);
    /// The reason and Undo (or Redo) at the end of a row.
    QWidget* finishRow(QWidget* w, QHBoxLayout* layout, qsizetype k, bool undone, const QString& why);

    QVBoxLayout* m_rows = nullptr;
    Tokens m_tokens;
    bool m_resizable = false;
    int m_fitHeight = 0;
};

} // namespace paraphrase::app
