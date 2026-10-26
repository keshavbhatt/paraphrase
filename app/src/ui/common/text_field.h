#pragma once

#include <QTextEdit>

namespace paraphrase::app {

/// A plain-text field with the mocks' reading line height. QPlainTextEdit lays out lines at
/// the font's own spacing and ignores a block's line height, so the user's text sat tighter
/// than the result beside it; this keeps every block at the given height, typed or pasted.
class TextField : public QTextEdit
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TextField)

public:
    /// The text pane's 15px/1.7 (main-window.html .pb).
    static constexpr int kTextLineHeight = 125;
    /// The form fields' 15px/1.6 (main-window-reply.html textarea.field).
    static constexpr int kFieldLineHeight = 118;

    explicit TextField(int lineHeight = kFieldLineHeight, QWidget* parent = nullptr);
    ~TextField() override = default;

private:
    void keepLineHeight();

    int m_lineHeight;
    bool m_applying = false;
};

} // namespace paraphrase::app
