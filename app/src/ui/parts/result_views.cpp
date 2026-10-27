#include "ui/parts/result_views.h"

#include "ui/common/chip_button.h"
#include "ui/common/grip_splitter.h"
#include "ui/common/widgets.h"
#include "ui/parts/change_list.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QTextBlockFormat>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
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
constexpr int kReplyGap = 10;
constexpr int kPopupReplyGap = 12;
constexpr int kArrowGap = 4;

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

/// The caret at the end of text that is still being written (main-window-busy.html, b): a
/// blinking accent bar over the text box, which has no focus and so no caret of its own.
class StreamCaret : public QWidget
{
public:
    explicit StreamCaret(QWidget* viewport)
        : QWidget(viewport)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        m_blink.setInterval(kBlinkMs);
        QObject::connect(&m_blink, &QTimer::timeout, this, [this] {
            m_on = !m_on;
            update();
        });
        hide();
    }

    /// At the end of the text: `cursor` is the text box's cursor rectangle there.
    void showAt(const QRect& cursor)
    {
        setGeometry(cursor.left(), cursor.top(), kWidth, cursor.height());
        m_on = true;
        show();
        raise();
        if (!m_blink.isActive()) {
            m_blink.start();
        }
        update();
    }

    void stop()
    {
        m_blink.stop();
        hide();
    }

    void setColour(const QColor& colour) { m_colour = colour; }

protected:
    void paintEvent(QPaintEvent* /*event*/) override
    {
        if (m_on) {
            QPainter(this).fillRect(rect(), m_colour);
        }
    }

private:
    static constexpr int kWidth = 2;
    static constexpr int kBlinkMs = 500; // mock.css: blink 1s, half on
    QTimer m_blink;
    QColor m_colour;
    bool m_on = true;
};

TextResultView::TextResultView(QWidget* parent)
    : QWidget(parent)
    , m_output(new QTextBrowser)
    , m_caret(new StreamCaret(m_output->viewport()))
    , m_changes(new ChangeList)
{
    QVBoxLayout* layout = viewLayout(this, 12);
    m_output->setObjectName(u"output"_s);
    m_output->setOpenLinks(false);
    m_output->document()->setDocumentMargin(0); // the pane's 14 px is the whole inset
    m_output->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere); // long links wrap too
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
    m_caret->stop();
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
    m_caret->showAt(m_output->cursorRect());
}

void TextResultView::endStream()
{
    m_caret->stop();
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
    // Until the user drags the split, the list is as tall as its rows need.
    if (m_split != nullptr && !QSettings().contains(m_splitKey)) {
        const int fit = m_changes->fitHeight();
        m_split->setSizes({std::max(1, m_split->height() - fit - m_split->handleWidth()), fit});
    }
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
    // The box hugs its text at the top; centred, it left an empty band above a short result.
    layout()->setAlignment(fit ? Qt::AlignTop : Qt::Alignment());
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

void TextResultView::splitChanges(const QString& settingsKey)
{
    m_splitKey = settingsKey;
    layout()->removeWidget(m_output);
    layout()->removeWidget(m_changes);
    m_changes->setResizable(true);
    m_split = new GripSplitter(Qt::Vertical);
    m_split->setObjectName(u"changesSplit"_s);
    m_split->stack(m_output, m_changes, settingsKey, GripSplitter::Start::BottomSmallest);
    layout()->addWidget(m_split);
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
    m_caret->setColour(t.accent);
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
    // Result is the corrected text alone, over the whole height; the list with Undo belongs to
    // Changes (main-window.html, approved 2026-10-27).
    if (m_showResult) {
        m_changes->hide();
    }
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
        if (s.before.trimmed().isEmpty() && s.after.trimmed().isEmpty()) {
            // Only spacing changed: the new spacing, as Result shows it. Struck-out line breaks
            // drew as a tall empty gap; the change list names it and can undo it.
            body += html(s.after);
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
    layout->addWidget(buildMore());
    layout->addSpacing(4);
    m_editLabel = makeLabel(tr("Edit before copying"), u"small"_s);
    layout->addWidget(m_editLabel);
    m_edit->setObjectName(u"replyEdit"_s);
    layout->addWidget(m_edit);
    layout->addStretch();
}

QWidget* QuickRepliesView::buildMore()
{
    // popup-open-in-window.html, note 3: quick replies are one-liners, an email wants more.
    m_more = new QFrame;
    m_more->setObjectName(u"moreRow"_s);
    auto* row = new QHBoxLayout(m_more);
    row->setContentsMargins(12, 4, 6, 4);
    row->setSpacing(8);
    QLabel* ask = makeLabel(tr("Need more than a line?"), u"muted"_s);
    row->addWidget(ask, 1);
    m_fullReply = makeButton(tr("Write a full reply in the window"), u"link"_s);
    m_fullReply->setObjectName(u"fullReplyInWindow"_s);
    m_fullReply->setLayoutDirection(Qt::RightToLeft); // the arrow after the words
    row->addWidget(m_fullReply);
    connect(m_fullReply, &QPushButton::clicked, this, &QuickRepliesView::fullReplyRequested);
    m_more->hide(); // the popup's: the window has its own Full reply switch
    return m_more;
}

void QuickRepliesView::setForPopup(bool popup)
{
    m_more->setVisible(popup);
    // popup-reply.html: full-width rows (Insert and Copy are below) and the reply goes into
    // the other app, so it is edited "before sending".
    for (QPushButton* copy : std::as_const(m_copies)) {
        copy->setVisible(!popup);
    }
    m_editLabel->setText(popup ? tr("Edit before sending") : tr("Edit before copying"));
    layout()->setSpacing(popup ? kPopupReplyGap : kReplyGap); // popup-reply.html .stack: 12
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
    m_fullReply->setIcon(
        Theme::icon(u"forward"_s, t.accent, kIconSize, kArrowGap, 0)); // 4 px after the words
    m_fullReply->setIconSize(QSize(kIconSize + kArrowGap, kIconSize));
}

FullReplyView::FullReplyView(QWidget* parent)
    : QWidget(parent)
    , m_edit(new QPlainTextEdit)
    , m_caret(new StreamCaret(m_edit->viewport()))
{
    QVBoxLayout* layout = viewLayout(this);
    m_edit->setObjectName(u"draftOut"_s);
    m_edit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
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
    m_caret->showAt(m_edit->cursorRect());
}

void FullReplyView::endStream()
{
    m_caret->stop();
}

void FullReplyView::applyTheme(const Tokens& t)
{
    m_caret->setColour(t.accent);
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
    m_caret->stop();
    m_edit->setPlainText(m_versions.at(index));
    Q_EMIT versionsChanged();
}

void FullReplyView::reset()
{
    m_versions.clear();
    m_index = -1;
    m_caret->stop();
    m_edit->clear();
    Q_EMIT versionsChanged();
}

QString FullReplyView::text() const
{
    return m_edit->toPlainText();
}

} // namespace paraphrase::app
