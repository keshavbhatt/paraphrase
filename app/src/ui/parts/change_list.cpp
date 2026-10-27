#include "ui/parts/change_list.h"

#include "ui/common/widgets.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kMaxHeight = 220;
constexpr int kTwoRows = 66; ///< two 30 px rows and their gap, plus the frame
constexpr int kArrowSize = 14;

QLabel* chip(const QString& text, const QString& kind)
{
    auto* l = new QLabel(text.isEmpty() ? u" "_s : text);
    l->setProperty("chip", kind);
    l->setTextFormat(Qt::PlainText);
    l->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed); // hugs its text, as in the mock
    return l;
}

} // namespace

ChangeList::ChangeList(QWidget* parent)
    : QScrollArea(parent)
{
    setObjectName(u"changes"_s);
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* content = new QWidget;
    m_rows = new QVBoxLayout(content);
    m_rows->setContentsMargins(0, 0, 0, 0);
    m_rows->setSpacing(4); // .changes rows: 30 px each, 34 apart (main-window.html)
    setWidget(content);
    setVisible(false);
}

void ChangeList::setDiff(const client::WordDiff* diff, const QSet<qsizetype>& undone)
{
    while (const QLayoutItem* item = m_rows->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    const auto changes = diff != nullptr ? diff->changes() : QList<qsizetype>();
    setVisible(!changes.isEmpty());
    if (changes.isEmpty()) {
        return;
    }
    for (const qsizetype k : changes) {
        m_rows->addWidget(row(*diff, k, undone.contains(k)));
    }
    m_rows->addStretch();
    // The list takes what its rows need, measured rather than assumed, up to kMaxHeight.
    // Measured from the rows themselves: rows just added to a visible list count as hidden
    // until Qt shows them, so the layout's own size hint would leave them out.
    int height = 0;
    for (int i = 0; i < m_rows->count(); ++i) {
        if (const QWidget* w = m_rows->itemAt(i)->widget()) {
            height += w->sizeHint().height() + m_rows->spacing();
        }
    }
    // At least that much too: in the popup the window fits its content, and a list squeezed
    // to two rows of five hid the rest behind a scrollbar.
    m_fitHeight = std::min(height + (2 * frameWidth()), kMaxHeight);
    if (m_resizable) {
        // In a split (the window): it starts at that height, and the user may drag it from
        // two rows up to the pane's height (main-window.html, note 8).
        setMinimumHeight(std::min(m_fitHeight, kTwoRows));
        setMaximumHeight(QWIDGETSIZE_MAX);
        return;
    }
    setMaximumHeight(m_fitHeight);
    setMinimumHeight(m_fitHeight);
}

namespace {

QString letters(const QString& word)
{
    QString out;
    for (const QChar c : word) {
        if (c.isLetterOrNumber() || c.isMark()) {
            out += c;
        }
    }
    return out;
}

QString punctuationName(const QString& mark)
{
    if (mark.isEmpty()) {
        return ChangeList::tr("spacing");
    }
    if (mark == u"."_s) {
        return ChangeList::tr("full stop");
    }
    if (mark == u","_s) {
        return ChangeList::tr("comma");
    }
    if (mark == u"?"_s) {
        return ChangeList::tr("question mark");
    }
    if (mark == u"'"_s || mark == u"’"_s) {
        return ChangeList::tr("apostrophe");
    }
    return ChangeList::tr("punctuation");
}

} // namespace

QString spacingName(const QString& space)
{
    const auto breaks = space.count(u'\n');
    if (breaks == 1) {
        return ChangeList::tr("a line break");
    }
    if (breaks > 1) {
        return ChangeList::tr("%1 line breaks").arg(breaks);
    }
    if (space.isEmpty()) {
        return ChangeList::tr("nothing");
    }
    return space.size() == 1 ? ChangeList::tr("a space") : ChangeList::tr("%1 spaces").arg(space.size());
}

namespace {

/// Levenshtein distance, for telling a spelling fix from another word.
qsizetype distance(const QString& a, const QString& b)
{
    QList<qsizetype> row(b.size() + 1);
    for (qsizetype j = 0; j <= b.size(); ++j) {
        row[j] = j;
    }
    for (qsizetype i = 1; i <= a.size(); ++i) {
        qsizetype diagonal = row[0];
        row[0] = i;
        for (qsizetype j = 1; j <= b.size(); ++j) {
            const qsizetype up = row[j];
            row[j] = std::min({row[j] + 1, row[j - 1] + 1, diagonal + (a.at(i - 1) == b.at(j - 1) ? 0 : 1)});
            diagonal = up;
        }
    }
    return row[b.size()];
}

} // namespace

QString changeReason(const QString& before, const QString& after)
{
    const QString from = letters(before);
    const QString to = letters(after);
    if (from.isEmpty() && to.isEmpty()) {
        // Only marks changed: name the one added, or the one taken away.
        const QString mark = after.trimmed().isEmpty() ? before.trimmed() : after.trimmed();
        return punctuationName(mark.right(1));
    }
    if (before.trimmed().isEmpty()) {
        return ChangeList::tr("added word");
    }
    if (after.trimmed().isEmpty()) {
        return ChangeList::tr("removed word");
    }
    if (from == to) {
        if (before.contains(u'\'') != after.contains(u'\'')) {
            return ChangeList::tr("spelling"); // egg's -> eggs, dont -> don't
        }
        const QString mark = after.size() > before.size() ? after.right(1) : before.right(1);
        return punctuationName(mark);
    }
    if (from.compare(to, Qt::CaseInsensitive) == 0) {
        return ChangeList::tr("capital letter");
    }
    const qsizetype longest = std::max(from.size(), to.size());
    return distance(from.toLower(), to.toLower()) <= std::max<qsizetype>(2, longest / 3)
               ? ChangeList::tr("spelling")
               : ChangeList::tr("wording");
}

QWidget* ChangeList::row(const client::WordDiff& diff, qsizetype k, bool undone)
{
    const auto& segments = diff.segments();
    const auto& s = segments.at(k);
    auto* w = new QWidget;
    auto* layout = new QHBoxLayout(w);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    const QString before = s.before.trimmed();
    const QString after = s.after.trimmed();
    if (before.isEmpty() && after.isEmpty()) {
        // Spacing only: "4 line breaks -> 2 line breaks", not "new -> removed".
        layout->addWidget(chip(spacingName(s.before), u"pill"_s));
        auto* arrow = new QLabel;
        arrow->setPixmap(
            Theme::icon(u"forward"_s, m_tokens.muted, kArrowSize).pixmap(kArrowSize, kArrowSize));
        layout->addWidget(arrow);
        layout->addWidget(chip(spacingName(s.after), u"pill"_s));
        return finishRow(w, layout, k, undone, tr("spacing"));
    }
    if (before.isEmpty()) {
        const bool atEnd = k + 1 >= segments.size() || segments.at(k + 1).after.startsWith(u'\n');
        layout->addWidget(chip(atEnd ? tr("end") : tr("new"), u"pill"_s));
    } else {
        layout->addWidget(chip(before, u"del"_s));
    }
    auto* arrow = new QLabel;
    arrow->setPixmap(Theme::icon(u"forward"_s, m_tokens.muted, kArrowSize).pixmap(kArrowSize, kArrowSize));
    layout->addWidget(arrow);
    layout->addWidget(after.isEmpty() ? chip(tr("removed"), u"pill"_s) : chip(after, u"ins"_s));
    return finishRow(w, layout, k, undone, changeReason(s.before, s.after));
}

QWidget* ChangeList::finishRow(QWidget* w, QHBoxLayout* layout, qsizetype k, bool undone, const QString& why)
{
    QLabel* reason = makeLabel(why, u"small"_s);
    reason->setObjectName(u"reason%1"_s.arg(k));
    layout->addWidget(reason);
    layout->addStretch(1);
    QPushButton* undo = makeButton(undone ? tr("Redo") : tr("Undo"), u"link"_s);
    undo->setObjectName(u"undo%1"_s.arg(k));
    connect(undo, &QPushButton::clicked, this, [this, k] { Q_EMIT undoToggled(k); });
    layout->addWidget(undo);
    return w;
}

} // namespace paraphrase::app
