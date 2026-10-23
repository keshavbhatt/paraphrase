#include "ui/parts/result_views.h"

#include "ui/common/chip_button.h"
#include "ui/common/widgets.h"
#include "ui/parts/change_list.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QTextBlockFormat>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kReplies = 3;
constexpr int kIconSize = 14;

QString html(const QString& text)
{
    return text.toHtmlEscaped().replace(u'\n', u"<br>"_s);
}

constexpr int kLineHeight = 125;

/// Plain text at the mocks' reading line height (mock.css: 15px/1.7). Qt scales the font's own
/// line spacing, about 1.36 em, so 1.7 em is kLineHeight percent.
void setReadableText(QTextBrowser* view, const QString& text)
{
    view->setPlainText(text);
    QTextCursor all(view->document());
    all.select(QTextCursor::Document);
    QTextBlockFormat format;
    format.setLineHeight(kLineHeight, QTextBlockFormat::ProportionalHeight);
    all.mergeBlockFormat(format);
}

QVBoxLayout* viewLayout(QWidget* view, int top = 14)
{
    auto* layout = new QVBoxLayout(view);
    layout->setContentsMargins(14, top, 14, 14);
    layout->setSpacing(10);
    return layout;
}

} // namespace

TextResultView::TextResultView(QWidget* parent)
    : QWidget(parent)
    , m_output(new QTextBrowser)
    , m_changes(new ChangeList)
{
    QVBoxLayout* layout = viewLayout(this, 12);
    m_output->setObjectName(u"output"_s);
    m_output->setOpenLinks(false);
    m_output->document()->setDocumentMargin(0); // the pane's 14 px is the whole inset
    connect(m_output->document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged, this,
            &TextResultView::fitHeight);
    layout->addWidget(m_output, 3);
    layout->addWidget(m_changes, 2);
    connect(m_changes, &ChangeList::undoToggled, this, [this](qsizetype k) {
        if (!m_undone.remove(k)) {
            m_undone.insert(k);
        }
        render();
        Q_EMIT diffChanged();
    });
}

void TextResultView::clear()
{
    m_diff.reset();
    m_undone.clear();
    m_result.clear();
    m_output->clear();
    m_changes->setDiff(nullptr, {});
}

void TextResultView::stream(const QString& soFar)
{
    setReadableText(m_output, soFar);
    m_output->moveCursor(QTextCursor::End);
}

void TextResultView::showText(const QString& text)
{
    clear();
    m_result = text.trimmed();
    setReadableText(m_output, m_result);
}

void TextResultView::showProofread(const QString& before, const QString& after)
{
    // The model returns the whole corrected text; the diff shows what changed (PLAN D6).
    QString corrected = after;
    if (!before.endsWith(u'\n') && corrected.endsWith(u'\n')) {
        corrected.chop(1);
    }
    clear();
    m_diff.emplace(before, corrected);
    render();
}

void TextResultView::showMessage(const QString& message, bool problem)
{
    clear();
    const QColor colour = problem ? m_tokens.danger : m_tokens.muted;
    m_output->setHtml(u"<p style=\"color:%1\">%2</p>"_s.arg(colour.name(), html(message)));
}

void TextResultView::setFitToText(bool fit)
{
    m_fit = fit;
    fitHeight();
}

void TextResultView::fitHeight()
{
    if (!m_fit) {
        return;
    }
    constexpr int kMaxFitHeight = 220;
    const int chrome = m_output->height() - m_output->viewport()->height();
    const int text = static_cast<int>(std::ceil(m_output->document()->size().height()));
    m_output->setFixedHeight(std::min(text + chrome, kMaxFitHeight));
}

void TextResultView::setShowResult(bool result)
{
    m_showResult = result;
    render();
}

qsizetype TextResultView::changeCount() const
{
    return m_diff ? m_diff->changes().size() : 0;
}

void TextResultView::applyTheme(const Tokens& t)
{
    m_tokens = t;
    m_changes->applyTheme(t);
    render();
}

void TextResultView::render()
{
    if (!m_diff) {
        return;
    }
    const client::WordDiff& diff = *m_diff;
    m_result = diff.result(m_undone);
    if (m_showResult) {
        setReadableText(m_output, m_result);
    } else {
        m_output->setHtml(
            u"<div style=\"white-space: pre-wrap; font-size: 15px; line-height: %1%\">%2</div>"_s
                .arg(kLineHeight)
                .arg(diffHtml(diff)));
    }
    m_changes->setDiff(&diff, m_undone);
}

QString TextResultView::diffHtml(const client::WordDiff& diff) const
{
    // Colour is never the only signal: deletions are struck out, insertions underlined.
    const QString del = u"<span style=\"color:%1; background:%2; text-decoration:line-through\">%3</span>"_s;
    const QString ins = u"<span style=\"color:%1; background:%2; text-decoration:underline\">%3</span>"_s;
    QString body;
    const auto& segments = diff.segments();
    for (qsizetype k = 0; k < segments.size(); ++k) {
        const auto& s = segments.at(k);
        if (!s.changed || m_undone.contains(k)) {
            body += html(s.changed ? s.before : s.after);
            continue;
        }
        if (!s.before.isEmpty()) {
            body += del.arg(m_tokens.del.name(), m_tokens.delBg.name(), html(s.before));
        }
        if (!s.before.isEmpty() && !s.after.isEmpty()) {
            body += u" "_s; // the old and the new word side by side, as in the mock
        }
        if (!s.after.isEmpty()) {
            body += ins.arg(m_tokens.ins.name(), m_tokens.insBg.name(), html(s.after));
        }
    }
    return body;
}

QuickRepliesView::QuickRepliesView(QWidget* parent)
    : QWidget(parent)
    , m_edit(new QLineEdit)
{
    QVBoxLayout* layout = viewLayout(this);
    for (int i = 0; i < kReplies; ++i) {
        auto* choice = new ChipButton(QString(), QString::number(i + 1));
        choice->setProperty("kind", u"choice"_s);
        choice->setObjectName(u"reply%1"_s.arg(i + 1));
        choice->setCheckable(true);
        choice->setAutoExclusive(true);
        choice->setWordWrap(true);
        connect(choice, &QPushButton::clicked, this, [this, choice] { m_edit->setText(choice->label()); });
        QPushButton* copy = makeButton(QString(), u"icon"_s);
        copy->setToolTip(tr("Copy reply %1").arg(i + 1));
        copy->setAccessibleName(tr("Copy reply %1").arg(i + 1));
        connect(copy, &QPushButton::clicked, this,
                [choice] { QApplication::clipboard()->setText(choice->label()); });
        auto* row = new QHBoxLayout;
        row->setSpacing(10);
        row->addWidget(choice, 1);
        row->addWidget(copy);
        layout->addLayout(row);
        m_choices.append(choice);
        m_copies.append(copy);
        // Number keys 1 to 3 put a reply in the edit box (main-window-reply.html, note 3).
        const auto* key = new QShortcut(QKeySequence(Qt::Key_1 + i), this, nullptr, nullptr,
                                        Qt::WidgetWithChildrenShortcut);
        connect(key, &QShortcut::activated, choice, &QPushButton::click);
    }
    layout->addSpacing(4);
    layout->addWidget(makeLabel(tr("Edit before copying"), u"small"_s));
    m_edit->setObjectName(u"replyEdit"_s);
    layout->addWidget(m_edit);
    layout->addStretch();
}

void QuickRepliesView::setReplies(const QStringList& replies)
{
    for (qsizetype i = 0; i < m_choices.size(); ++i) {
        const QString text = replies.value(i);
        m_choices.at(i)->setLabel(text);
        m_choices.at(i)->setEnabled(!text.isEmpty());
        m_copies.at(i)->setEnabled(!text.isEmpty());
    }
    if (replies.isEmpty()) {
        m_edit->clear();
        return;
    }
    m_choices.front()->setChecked(true);
    m_edit->setText(replies.front());
}

QString QuickRepliesView::chosen() const
{
    return m_edit->text();
}

void QuickRepliesView::setChosen(const QString& text)
{
    m_edit->setText(text);
}

void QuickRepliesView::applyTheme(const Tokens& t)
{
    for (QPushButton* copy : std::as_const(m_copies)) {
        copy->setIcon(Theme::icon(u"copy"_s, t.text, kIconSize));
    }
}

FullReplyView::FullReplyView(QWidget* parent)
    : QWidget(parent)
    , m_edit(new QPlainTextEdit)
{
    QVBoxLayout* layout = viewLayout(this);
    m_edit->setObjectName(u"draftOut"_s);
    m_edit->setPlaceholderText(tr("Your reply appears here. Edit it before you copy it."));
    layout->addWidget(m_edit, 1);
    m_hint = makeLabel(tr("Edit it here before you copy it. Names, dates and times come only from the "
                          "message and from what you wrote."),
                       u"small"_s);
    m_hint->setWordWrap(true);
    layout->addWidget(m_hint);
}

void FullReplyView::setComposing(bool composing)
{
    m_hint->setVisible(!composing);
}

void FullReplyView::stream(const QString& soFar)
{
    m_edit->setPlainText(soFar);
    m_edit->moveCursor(QTextCursor::End);
}

void FullReplyView::addVersion(const QString& text)
{
    m_versions.append(text.trimmed());
    showVersion(m_versions.size() - 1);
}

void FullReplyView::showVersion(qsizetype index)
{
    if (index < 0 || index >= m_versions.size()) {
        return;
    }
    m_index = index;
    m_edit->setPlainText(m_versions.at(index));
    Q_EMIT versionsChanged();
}

void FullReplyView::reset()
{
    m_versions.clear();
    m_index = -1;
    m_edit->clear();
    Q_EMIT versionsChanged();
}

QString FullReplyView::text() const
{
    return m_edit->toPlainText();
}

} // namespace paraphrase::app
