#pragma once

#include "ui/common/theme.h"
#include "word_diff.h"

#include <QScrollArea>
#include <QSet>

class QVBoxLayout;

namespace paraphrase::app {

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

Q_SIGNALS:
    void undoToggled(qsizetype change);

private:
    [[nodiscard]] QWidget* row(const client::WordDiff& diff, qsizetype k, bool undone);

    QVBoxLayout* m_rows = nullptr;
    Tokens m_tokens;
};

} // namespace paraphrase::app
