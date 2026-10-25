#pragma once

#include "history/history_store.h"
#include "ui/common/theme.h"

#include <QWidget>

class QLabel;
class QLayout;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;

namespace paraphrase::app {

namespace history {
class Keeper;
}
class ChipButton;
class TextResultView;

/// History in the main window (main-window-history.html): the kept runs, newest first by day,
/// with search and action filters; the open entry's text and result, Copy result, Use again
/// and delete, asked on the page. While history is off it says so, with a way to turn it on.
class HistoryView : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(HistoryView)

public:
    explicit HistoryView(history::Keeper& keeper, QWidget* parent = nullptr);
    ~HistoryView() override = default;

    /// Reads the entries again (the view is opened, or the history changed).
    void reload();
    void applyTheme(const Tokens& t);

    // For tests.
    [[nodiscard]] int shownCount() const;
    [[nodiscard]] QString openResult() const;

Q_SIGNALS:
    void backRequested();
    /// The entry's text, back on the left with the same action and options.
    void useAgain(const history::Entry& entry);
    /// "Turn on history" and "Privacy settings".
    void privacyRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void buildList();
    void openEntry(int index);
    void copyResult();
    void askDelete(bool asking);
    [[nodiscard]] bool matches(const history::Entry& e) const;
    [[nodiscard]] QWidget* buildOffPage();
    [[nodiscard]] QWidget* buildPanes();
    [[nodiscard]] QWidget* buildSearch();
    [[nodiscard]] QLayout* buildChips();
    [[nodiscard]] QWidget* buildDetail();
    [[nodiscard]] QWidget* buildDetailHead();

    history::Keeper& m_keeper;
    QList<history::Entry> m_entries;
    QString m_filter; ///< a skill base, "" for All, "custom" for My actions
    int m_open = -1;  ///< index into m_entries

    QPushButton* m_back = nullptr;
    QPushButton* m_offBack = nullptr;
    QLabel* m_offIcon = nullptr;
    QLabel* m_count = nullptr;
    QLineEdit* m_search = nullptr;
    QLabel* m_searchIcon = nullptr;
    QList<QPushButton*> m_chips;
    QPushButton* m_more = nullptr;
    QListWidget* m_list = nullptr;
    QLabel* m_empty = nullptr;
    QWidget* m_detail = nullptr;
    QLabel* m_what = nullptr;
    QLabel* m_meta = nullptr;
    QPushButton* m_delete = nullptr;
    QWidget* m_confirm = nullptr;
    QPlainTextEdit* m_text = nullptr;
    QPlainTextEdit* m_result = nullptr;
    TextResultView* m_diff = nullptr; ///< a Proofread entry's result, as the window shows it
    QPushButton* m_copy = nullptr;
    QPushButton* m_again = nullptr;
    QLabel* m_lock = nullptr;
    QLabel* m_kept = nullptr;
    Tokens m_tokens;
    // Last: their builders fill the members above, which must exist by then.
    QWidget* m_off = nullptr;
    QWidget* m_on = nullptr;
};

} // namespace paraphrase::app
