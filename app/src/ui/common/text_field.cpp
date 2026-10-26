#include "ui/common/text_field.h"

#include <QTextBlock>
#include <QTextCursor>

namespace paraphrase::app {

TextField::TextField(int lineHeight, QWidget* parent)
    : QTextEdit(parent)
    , m_lineHeight(lineHeight)
{
    setAcceptRichText(false);
    setProperty("field", true);
    connect(this, &QTextEdit::textChanged, this, &TextField::keepLineHeight);
    keepLineHeight();
}

void TextField::keepLineHeight()
{
    if (m_applying) {
        return;
    }
    bool kept = true;
    for (QTextBlock b = document()->begin(); b.isValid(); b = b.next()) {
        const QTextBlockFormat f = b.blockFormat();
        if (f.lineHeightType() != QTextBlockFormat::ProportionalHeight || f.lineHeight() != m_lineHeight) {
            kept = false;
            break;
        }
    }
    if (kept) {
        return;
    }
    // New blocks take their neighbour's format, so this runs after setPlainText() or clear(),
    // which reset the document. A fresh document has no undo history: keep it that way, so
    // the first Ctrl+Z does not just undo the spacing.
    m_applying = true;
    const bool history = document()->isUndoAvailable();
    QTextCursor all(document());
    if (history) {
        all.joinPreviousEditBlock();
    } else {
        all.beginEditBlock();
    }
    all.select(QTextCursor::Document);
    QTextBlockFormat format;
    format.setLineHeight(m_lineHeight, QTextBlockFormat::ProportionalHeight);
    all.mergeBlockFormat(format);
    all.endEditBlock();
    if (!history) {
        document()->clearUndoRedoStacks();
    }
    m_applying = false;
}

} // namespace paraphrase::app
