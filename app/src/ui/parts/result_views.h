#pragma once

#include "ui/common/theme.h"
#include "word_diff.h"

#include <QSet>
#include <QStringList>
#include <QWidget>

#include <optional>

class QFrame;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTextBrowser;

namespace paraphrase::app {

class ChangeList;
class ChipButton;
class StreamCaret;

/// A text result: streamed as it is written, then shown whole; for proofread the word diff
/// with Undo per change (PLAN D6) or the corrected text alone.
class TextResultView : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TextResultView)

public:
    explicit TextResultView(QWidget* parent = nullptr);
    ~TextResultView() override = default;

    void clear();
    void stream(const QString& soFar);
    /// The run ended without a final text (stopped): the caret goes.
    void endStream();
    void showText(const QString& text);
    void showProofread(const QString& before, const QString& after);
    void showMessage(const QString& message, bool problem);
    /// Changes (the diff) or the corrected text alone.
    void setShowResult(bool result);
    /// The text box as tall as its text, up to a cap, instead of filling the space (popup).
    void setFitToText(bool fit);
    [[nodiscard]] QString result() const { return m_result; }
    [[nodiscard]] bool hasDiff() const { return m_diff.has_value(); }
    [[nodiscard]] qsizetype changeCount() const;
    void applyTheme(const Tokens& t);

Q_SIGNALS:
    void diffChanged(); ///< a change was undone or redone

private:
    void render();
    void fitHeight();
    bool m_fit = false;
    [[nodiscard]] QString diffHtml(const client::WordDiff& diff) const;

    QTextBrowser* m_output = nullptr;
    StreamCaret* m_caret = nullptr;
    ChangeList* m_changes = nullptr;
    std::optional<client::WordDiff> m_diff;
    QSet<qsizetype> m_undone;
    QString m_result;
    bool m_showResult = false;
    Tokens m_tokens;
};

/// Three short replies to pick from, each with Copy, and an edit box (main-window-reply.html).
class QuickRepliesView : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QuickRepliesView)

public:
    explicit QuickRepliesView(QWidget* parent = nullptr);
    ~QuickRepliesView() override = default;

    /// In the popup: no copy button per row and "Edit before sending" (popup-reply.html).
    void setForPopup(bool popup);
    void setReplies(const QStringList& replies);
    /// The reply as edited: what Copy copies.
    [[nodiscard]] QString chosen() const;
    void setChosen(const QString& text);
    void applyTheme(const Tokens& t);

Q_SIGNALS:
    /// The popup's "Write a full reply in the window".
    void fullReplyRequested();

private:
    [[nodiscard]] QWidget* buildMore();

    QFrame* m_more = nullptr;
    QPushButton* m_fullReply = nullptr;
    QLabel* m_editLabel = nullptr;
    QList<ChipButton*> m_choices;
    QList<QPushButton*> m_copies;
    QLineEdit* m_edit = nullptr;
};

/// One full reply, editable, with the versions written so far (main-window-reply-draft.html).
class FullReplyView : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FullReplyView)

public:
    explicit FullReplyView(QWidget* parent = nullptr);
    ~FullReplyView() override = default;

    void stream(const QString& soFar);
    void endStream();
    void applyTheme(const Tokens& t);
    void addVersion(const QString& text);
    void showVersion(qsizetype index);
    /// Forgets the versions: the message or what to say changed.
    void reset();
    /// Write has no message to take facts from: its view shows no reply hint
    /// (main-window-compose.html).
    void setComposing(bool composing);
    [[nodiscard]] QString text() const;
    [[nodiscard]] qsizetype versionCount() const { return m_versions.size(); }
    [[nodiscard]] qsizetype versionIndex() const { return m_index; }

Q_SIGNALS:
    void versionsChanged();

private:
    QPlainTextEdit* m_edit = nullptr;
    StreamCaret* m_caret = nullptr;
    QLabel* m_hint = nullptr;
    QStringList m_versions;
    qsizetype m_index = -1;
};

} // namespace paraphrase::app
