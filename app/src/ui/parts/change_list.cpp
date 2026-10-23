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
    m_rows->setSpacing(2);
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
    setMaximumHeight(std::min(height + (2 * frameWidth()), kMaxHeight));
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
    layout->addStretch(1);
    QPushButton* undo = makeButton(undone ? tr("Redo") : tr("Undo"), u"link"_s);
    undo->setObjectName(u"undo%1"_s.arg(k));
    connect(undo, &QPushButton::clicked, this, [this, k] { Q_EMIT undoToggled(k); });
    layout->addWidget(undo);
    return w;
}

} // namespace paraphrase::app
