#include "ui/main/history_view.h"

#include "history/history_keeper.h"
#include "ui/common/chip_button.h"
#include "ui/common/menu_icons.h"
#include "ui/common/widgets.h"
#include "ui/parts/result_views.h"

#include <QApplication>
#include <QClipboard>
#include <QDate>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kEntryRole = Qt::UserRole;   ///< index into the entries, -1 for a day
constexpr int kDayRole = Qt::UserRole + 1; ///< a day's heading
constexpr int kDayHeight = 30;
constexpr int kEntryHeight = 54;
constexpr int kBadge = 26;
constexpr int kSmallIcon = 14;

struct Filter
{
    QString base; ///< "" for All
    QString name;
};

QList<Filter> chipFilters()
{
    return {
        Filter{.base = {}, .name = HistoryView::tr("All")},
        Filter{.base = u"proofread"_s, .name = HistoryView::tr("Proofread")},
        Filter{.base = u"rewrite"_s, .name = HistoryView::tr("Tone")},
        Filter{.base = u"reply"_s, .name = HistoryView::tr("Reply")},
    };
}

QList<Filter> moreFilters()
{
    return {
        Filter{.base = u"compose"_s, .name = HistoryView::tr("Write")},
        Filter{.base = u"summarize"_s, .name = HistoryView::tr("Summarize")},
        Filter{.base = u"format"_s, .name = HistoryView::tr("Format")},
        Filter{.base = u"translate"_s, .name = HistoryView::tr("Translate")},
        Filter{.base = u"custom"_s, .name = HistoryView::tr("My actions")},
    };
}

QString baseOf(const QString& skillId)
{
    const QString base = skillId.section(u'.', 0, 0);
    return base == u"draft"_s ? u"reply"_s : base; // a full reply is a Reply
}

QString badgeIcon(const QString& skillId)
{
    const QString base = skillId.section(u'.', 0, 0);
    QString preset = presetIcon(skillId.section(u'.', 1));
    if (base == u"proofread"_s) {
        return u"check"_s;
    }
    if (base == u"reply"_s || base == u"draft"_s) {
        return u"forward"_s;
    }
    if (base == u"compose"_s) {
        return u"mail"_s;
    }
    if (base == u"summarize"_s) {
        return u"shrink"_s;
    }
    if (!preset.isEmpty()) {
        return preset;
    }
    return base == u"rewrite"_s ? u"smile"_s : u"pen"_s;
}

QString dayName(const QDate& day)
{
    const QDate today = QDate::currentDate();
    if (day == today) {
        return HistoryView::tr("Today");
    }
    if (day == today.addDays(-1)) {
        return HistoryView::tr("Yesterday");
    }
    const QString format = day.year() == today.year() ? u"dddd d MMMM"_s : u"d MMMM yyyy"_s;
    return QLocale().toString(day, format);
}

QString timeText(const QDateTime& when)
{
    return QLocale().toString(when.time(), QLocale::ShortFormat);
}

QString oneLine(const QString& text)
{
    return text.simplified();
}

/// Paints the list (main-window-history.html .day, .entry): a day's heading, or a badge, the
/// action, the time and the start of the result.
class EntryDelegate : public QStyledItemDelegate
{
public:
    EntryDelegate(const QList<history::Entry>& entries, const Tokens& tokens, QObject* parent)
        : QStyledItemDelegate(parent)
        , m_entries(entries)
        , m_tokens(tokens)
    {}

    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        const bool day = index.data(kEntryRole).toInt() < 0;
        return {option.rect.width(), day ? kDayHeight : kEntryHeight};
    }

    void paint(QPainter* p, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        const int i = index.data(kEntryRole).toInt();
        const Tokens& t = m_tokens;
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        const QRect r = option.rect;
        QFont font = option.font;
        if (i < 0 || i >= m_entries.size()) {
            font.setPixelSize(11);
            font.setWeight(QFont::Bold);
            font.setLetterSpacing(QFont::AbsoluteSpacing, 0.7);
            font.setCapitalization(QFont::AllUppercase);
            p->setFont(font);
            p->setPen(t.muted);
            p->drawText(r.adjusted(2, 8, 0, -2), Qt::AlignLeft | Qt::AlignBottom,
                        index.data(kDayRole).toString());
            p->restore();
            return;
        }
        const history::Entry& e = m_entries.at(i);
        const bool selected = (option.state & QStyle::State_Selected) != 0;
        const bool hover = (option.state & QStyle::State_MouseOver) != 0;
        if (selected || hover) {
            QPainterPath path;
            path.addRoundedRect(QRectF(r), 8, 8);
            p->fillPath(path, selected ? t.accentSoft : t.hover);
        }
        const QRect badge(r.left() + 10, r.top() + ((r.height() - kBadge) / 2), kBadge, kBadge);
        QPainterPath badgePath;
        badgePath.addRoundedRect(QRectF(badge), 7, 7);
        p->fillPath(badgePath, selected ? t.panel : t.hover);
        Theme::icon(badgeIcon(e.skillId), selected ? t.accent : t.muted, kSmallIcon)
            .paint(p, badge.adjusted(6, 6, -6, -6));

        const int left = badge.right() + 11;
        const QRect lines(left, r.top() + 9, r.right() - 10 - left, r.height() - 18);
        QFont small = font;
        small.setPixelSize(12);
        p->setFont(small);
        p->setPen(t.muted);
        const QString when = timeText(e.when);
        const int whenWidth = QFontMetrics(small).horizontalAdvance(when);
        p->drawText(lines, Qt::AlignRight | Qt::AlignTop, when);
        QFont strong = small;
        strong.setWeight(QFont::DemiBold);
        p->setFont(strong);
        p->setPen(t.text);
        const QString what = e.app.isEmpty() ? e.label : e.label + u" · "_s + e.app;
        p->drawText(lines.adjusted(0, 0, -whenWidth - 10, 0), Qt::AlignLeft | Qt::AlignTop,
                    QFontMetrics(strong).elidedText(what, Qt::ElideRight, lines.width() - whenWidth - 10));
        QFont body = font;
        body.setPixelSize(13);
        p->setFont(body);
        p->setPen(t.muted);
        p->drawText(lines, Qt::AlignLeft | Qt::AlignBottom,
                    QFontMetrics(body).elidedText(oneLine(e.result), Qt::ElideRight, lines.width()));
        p->restore();
    }

private:
    const QList<history::Entry>& m_entries;
    const Tokens& m_tokens;
};

/// The mock's `.cap`: a small uppercase label over a box.
QLabel* caption(const QString& text)
{
    QLabel* label = makeLabel(text, u"day"_s);
    QFont f = label->font();
    f.setCapitalization(QFont::AllUppercase);
    label->setFont(f);
    return label;
}

QPlainTextEdit* makeBox(const QString& name, bool sunk)
{
    auto* box = new QPlainTextEdit;
    box->setObjectName(name);
    box->setReadOnly(true);
    box->setProperty("role", u"box"_s);
    box->setProperty("sunk", sunk);
    box->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    return box;
}

} // namespace

HistoryView::HistoryView(history::Keeper& keeper, QWidget* parent)
    : QWidget(parent)
    , m_keeper(keeper)
    , m_lock(new QLabel)
    , m_off(buildOffPage())
    , m_on(buildPanes())
{
    setObjectName(u"historyView"_s);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    root->addWidget(m_off, 1);
    root->addWidget(m_on, 1);
    // One of the two from the start: the window's height goes by the view's while it is hidden.
    m_off->setVisible(!history::Keeper::isOn());
    m_on->setVisible(history::Keeper::isOn());

    // The status band: where it lives, always said (note 5).
    auto* status = new QFrame;
    status->setObjectName(u"statusBar"_s);
    auto* band = new QHBoxLayout(status);
    band->setContentsMargins(16, 7, 16, 7);
    band->setSpacing(6);

    m_kept = makeLabel({}, u"private"_s);
    m_kept->setObjectName(u"historyKeptLine"_s);
    band->addWidget(m_lock);
    band->addWidget(m_kept);
    band->addStretch();
    QPushButton* privacy = makeButton(tr("Privacy settings"), u"link"_s);
    privacy->setObjectName(u"privacySettings"_s);
    band->addWidget(privacy);
    root->addWidget(status);

    connect(privacy, &QPushButton::clicked, this, &HistoryView::privacyRequested);
    connect(&m_keeper, &history::Keeper::changed, this, [this] {
        if (isVisible()) {
            reload();
        }
    });
}

QWidget* HistoryView::buildOffPage()
{
    auto* page = new QWidget;
    page->setObjectName(u"historyOff"_s);
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(40, 40, 40, 40);
    outer->addStretch();
    auto* column = new QVBoxLayout;
    column->setSpacing(10);
    m_offIcon = new QLabel;
    column->addWidget(m_offIcon, 0, Qt::AlignHCenter);
    column->addWidget(makeLabel(tr("History is off"), u"h2"_s), 0, Qt::AlignHCenter);
    QLabel* text =
        makeLabel(tr("Nothing you write is kept. Turn history on to find past results here and copy "
                     "or reuse them. It stays encrypted on this computer."),
                  u"muted"_s);
    text->setWordWrap(true);
    text->setAlignment(Qt::AlignHCenter);
    // Centred between stretches, not with an alignment flag: that would drop the label's
    // height for its width and show one line of it.
    text->setFixedWidth(460);
    auto* textRow = new QHBoxLayout;
    textRow->addStretch();
    textRow->addWidget(text);
    textRow->addStretch();
    column->addLayout(textRow);
    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(8);
    m_offBack = makeButton(tr("Back to writing"), u"flat"_s);
    m_offBack->setObjectName(u"offBack"_s);
    QPushButton* turnOn = makeButton(tr("Turn on history"), u"primary"_s);
    turnOn->setObjectName(u"turnOnHistory"_s);
    buttons->addWidget(m_offBack);
    buttons->addWidget(turnOn);
    column->addLayout(buttons);
    column->setAlignment(buttons, Qt::AlignHCenter);
    outer->addLayout(column);
    outer->addStretch();
    connect(m_offBack, &QPushButton::clicked, this, &HistoryView::backRequested);
    connect(turnOn, &QPushButton::clicked, this, &HistoryView::privacyRequested);
    return page;
}

QWidget* HistoryView::buildPanes()
{
    auto* panes = new QWidget;
    auto* grid = new QHBoxLayout(panes);
    grid->setContentsMargins(16, 14, 16, 16);
    grid->setSpacing(14);

    // Entries.
    auto* list = new QFrame;
    list->setObjectName(u"pane"_s);
    list->setMinimumWidth(280);
    list->setMaximumWidth(380);
    auto* listColumn = new QVBoxLayout(list);
    listColumn->setContentsMargins(0, 0, 0, 0);
    listColumn->setSpacing(0);
    auto* listHead = new QFrame;
    listHead->setObjectName(u"paneHead"_s);
    auto* head = new QHBoxLayout(listHead);
    head->setContentsMargins(14, 10, 14, 10);
    head->setSpacing(8);
    m_back = makeButton(tr("Back to writing"), u"flat"_s);
    m_back->setObjectName(u"backToWriting"_s);
    m_count = makeLabel({}, u"small"_s);
    head->addWidget(m_back);
    head->addStretch();
    head->addWidget(m_count);
    listColumn->addWidget(listHead);
    auto* listBody = new QVBoxLayout;
    listBody->setContentsMargins(14, 14, 14, 14);
    listBody->setSpacing(10);
    listBody->addWidget(buildSearch());
    listBody->addLayout(buildChips());
    m_list = new QListWidget;
    m_list->setObjectName(u"historyList"_s);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setMouseTracking(true);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setItemDelegate(new EntryDelegate(m_entries, m_tokens, m_list));
    listBody->addWidget(m_list, 1);
    m_empty = makeLabel(tr("Nothing kept yet. Finished results land here."), u"muted"_s);
    m_empty->setObjectName(u"historyEmpty"_s);
    m_empty->setWordWrap(true);
    m_empty->setAlignment(Qt::AlignCenter);
    listBody->addWidget(m_empty, 1);
    listColumn->addLayout(listBody, 1);
    grid->addWidget(list, 1);
    grid->addWidget(buildDetail(), 2);

    connect(m_back, &QPushButton::clicked, this, &HistoryView::backRequested);
    connect(m_search, &QLineEdit::textChanged, this, &HistoryView::buildList);
    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row) {
        const QListWidgetItem* item = m_list->item(row);
        openEntry(item == nullptr ? -1 : item->data(kEntryRole).toInt());
    });
    return panes;
}

QWidget* HistoryView::buildSearch()
{
    // main-window-history.html .search: the icon and the field in one rounded box.
    auto* searchBox = new QFrame;
    searchBox->setObjectName(u"search"_s);
    searchBox->setMinimumHeight(36);
    auto* searchRow = new QHBoxLayout(searchBox);
    searchRow->setContentsMargins(10, 0, 6, 0);
    searchRow->setSpacing(8);
    m_searchIcon = new QLabel;
    m_search = new QLineEdit;
    m_search->setObjectName(u"historySearch"_s);
    m_search->setPlaceholderText(tr("Search your history"));
    m_search->setClearButtonEnabled(true);
    searchRow->addWidget(m_searchIcon);
    searchRow->addWidget(m_search, 1);
    return searchBox;
}

QLayout* HistoryView::buildChips()
{
    auto* chips = new QHBoxLayout;
    chips->setSpacing(6);
    for (const Filter& f : chipFilters()) {
        QPushButton* chip = makeButton(f.name, u"filter"_s);
        chip->setCheckable(true);
        chip->setChecked(f.base.isEmpty());
        chip->setObjectName(u"filter-"_s + (f.base.isEmpty() ? u"all"_s : f.base));
        connect(chip, &QPushButton::clicked, this, [this, chip, base = f.base] {
            m_filter = base;
            for (QPushButton* other : std::as_const(m_chips)) {
                other->setChecked(other == chip);
            }
            m_more->setChecked(false);
            m_more->setText(tr("More"));
            buildList();
        });
        m_chips.append(chip);
        chips->addWidget(chip);
    }
    m_more = makeButton(tr("More"), u"filter"_s);
    m_more->setObjectName(u"filter-more"_s);
    m_more->setCheckable(true);
    m_more->setLayoutDirection(Qt::RightToLeft); // the arrow after the text, as the mock
    auto* menu = new QMenu(m_more);
    for (const Filter& f : moreFilters()) {
        menu->addAction(f.name, this, [this, f] {
            m_filter = f.base;
            for (QPushButton* chip : std::as_const(m_chips)) {
                chip->setChecked(false);
            }
            m_more->setChecked(true);
            m_more->setText(f.name);
            buildList();
        });
    }
    connect(m_more, &QPushButton::clicked, this, [this, menu] {
        m_more->setChecked(
            !m_filter.isEmpty() && !m_chips.isEmpty() &&
            std::ranges::none_of(m_chips, [](const QPushButton* c) { return c->isChecked(); }));
        menu->popup(m_more->mapToGlobal(QPoint(0, m_more->height() + 4)));
    });
    chips->addWidget(m_more);
    chips->addStretch();
    return chips;
}

QWidget* HistoryView::buildDetailHead()
{
    auto* detailHead = new QFrame;
    detailHead->setObjectName(u"paneHead"_s);
    auto* dHead = new QHBoxLayout(detailHead);
    dHead->setContentsMargins(14, 10, 14, 10);
    dHead->setSpacing(8);
    m_what = makeLabel({}, u"strong"_s);
    m_what->setObjectName(u"entryWhat"_s);
    m_meta = makeLabel({}, u"small"_s);
    m_meta->setObjectName(u"entryMeta"_s);
    m_delete = makeButton({}, u"icon"_s);
    m_delete->setObjectName(u"deleteEntry"_s);
    m_delete->setAccessibleName(tr("Delete this entry"));
    m_delete->setToolTip(tr("Delete this entry"));
    dHead->addWidget(m_what);
    dHead->addWidget(m_meta, 1);
    dHead->addWidget(m_delete);
    m_confirm = new QWidget;
    auto* confirm = new QHBoxLayout(m_confirm);
    confirm->setContentsMargins(0, 0, 0, 0);
    confirm->setSpacing(6);
    confirm->addWidget(makeLabel(tr("Delete this entry?"), u"small"_s));
    QPushButton* keep = makeButton(tr("Keep"));
    keep->setObjectName(u"keepEntry"_s);
    QPushButton* remove = makeButton(tr("Delete"), u"dangerSolid"_s);
    remove->setObjectName(u"confirmDeleteEntry"_s);
    confirm->addWidget(keep);
    confirm->addWidget(remove);
    m_confirm->hide();
    dHead->addWidget(m_confirm);

    connect(m_delete, &QPushButton::clicked, this, [this] { askDelete(true); });
    connect(keep, &QPushButton::clicked, this, [this] { askDelete(false); });
    connect(remove, &QPushButton::clicked, this, [this] {
        if (m_open >= 0 && m_open < m_entries.size()) {
            const qint64 id = m_entries.at(m_open).id;
            askDelete(false);
            m_keeper.remove(id); // changed() reloads
        }
    });
    return detailHead;
}

QWidget* HistoryView::buildDetail()
{
    m_detail = new QFrame;
    m_detail->setObjectName(u"pane"_s);
    auto* detail = new QVBoxLayout(m_detail);
    detail->setContentsMargins(0, 0, 0, 0);
    detail->setSpacing(0);
    detail->addWidget(buildDetailHead());
    auto* body = new QVBoxLayout;
    body->setContentsMargins(14, 14, 14, 14);
    body->setSpacing(10);
    body->addWidget(caption(tr("Your text")));
    m_text = makeBox(u"entryText"_s, true);
    body->addWidget(m_text, 1);
    body->addWidget(caption(tr("Result")));
    m_result = makeBox(u"entryResult"_s, false);
    body->addWidget(m_result, 1);
    m_diff = new TextResultView;
    m_diff->setObjectName(u"entryDiff"_s);
    m_diff->hide();
    body->addWidget(m_diff, 1);
    detail->addLayout(body, 1);
    auto* foot = new QFrame;
    foot->setObjectName(u"paneFoot"_s);
    auto* footRow = new QHBoxLayout(foot);
    footRow->setContentsMargins(14, 10, 14, 10);
    footRow->setSpacing(8);
    auto* copy = new ChipButton(tr("Copy result"), tr("Ctrl Shift C"), ChipButton::ChipSide::After);
    copy->setProperty("kind", u"primary"_s);
    copy->setObjectName(u"copyEntryResult"_s);
    m_copy = copy;
    m_again = makeButton(tr("Use again"));
    m_again->setObjectName(u"useAgain"_s);
    footRow->addWidget(m_copy);
    footRow->addWidget(m_again);
    footRow->addStretch();
    detail->addWidget(foot);

    connect(m_copy, &QPushButton::clicked, this, &HistoryView::copyResult);
    connect(m_again, &QPushButton::clicked, this, [this] {
        if (m_open >= 0 && m_open < m_entries.size()) {
            Q_EMIT useAgain(m_entries.at(m_open));
        }
    });
    return m_detail;
}

void HistoryView::reload()
{
    const bool on = history::Keeper::isOn();
    m_off->setVisible(!on);
    m_on->setVisible(on);
    const int days = history::Keeper::keepDays();
    if (!on) {
        m_kept->setText(tr("History is off: nothing you write is kept"));
    } else if (days > 0) {
        m_kept->setText(days == 365 ? tr("Kept encrypted on this computer for a year")
                                    : tr("Kept encrypted on this computer for %1 days").arg(days));
    } else {
        m_kept->setText(tr("Kept encrypted on this computer until you delete it"));
    }
    if (!on) {
        m_entries.clear();
        m_open = -1;
        m_list->clear();
        return;
    }
    const qint64 openId = m_open >= 0 && m_open < m_entries.size() ? m_entries.at(m_open).id : 0;
    m_entries = m_keeper.entries();
    m_open = -1;
    buildList();
    for (int row = 0; row < m_list->count(); ++row) {
        const int i = m_list->item(row)->data(kEntryRole).toInt();
        if (i >= 0 && m_entries.at(i).id == openId) {
            m_list->setCurrentRow(row);
        }
    }
}

bool HistoryView::matches(const history::Entry& e) const
{
    if (!m_filter.isEmpty() && baseOf(e.skillId) != m_filter) {
        return false;
    }
    const QString q = m_search->text().trimmed();
    return q.isEmpty() || e.text.contains(q, Qt::CaseInsensitive) ||
           e.result.contains(q, Qt::CaseInsensitive) || e.label.contains(q, Qt::CaseInsensitive) ||
           e.app.contains(q, Qt::CaseInsensitive);
}

void HistoryView::buildList()
{
    const qint64 openId = m_open >= 0 && m_open < m_entries.size() ? m_entries.at(m_open).id : 0;
    const QSignalBlocker block(m_list);
    m_list->clear();
    QDate day;
    int shown = 0;
    int reopen = -1;
    for (int i = 0; i < m_entries.size(); ++i) {
        const history::Entry& e = m_entries.at(i);
        if (!matches(e)) {
            continue;
        }
        if (e.when.date() != day) {
            day = e.when.date();
            auto* heading = new QListWidgetItem(m_list);
            heading->setData(kEntryRole, -1);
            heading->setData(kDayRole, dayName(day));
            heading->setFlags(Qt::NoItemFlags);
        }
        auto* item = new QListWidgetItem(m_list);
        item->setData(kEntryRole, i);
        item->setData(Qt::AccessibleTextRole, e.label + u", "_s + oneLine(e.result).left(80));
        if (e.id == openId) {
            reopen = m_list->count() - 1;
        }
        ++shown;
    }
    m_count->setText(m_entries.size() == 1 ? tr("1 entry") : tr("%1 entries").arg(m_entries.size()));
    m_list->setVisible(shown > 0);
    m_empty->setVisible(shown == 0);
    m_empty->setText(m_entries.isEmpty() ? tr("Nothing kept yet. Finished results land here.")
                                         : tr("No entry matches."));
    // The newest match opens, or the one that was open.
    int row = reopen;
    for (int r = 0; row < 0 && r < m_list->count(); ++r) {
        if (m_list->item(r)->data(kEntryRole).toInt() >= 0) {
            row = r;
        }
    }
    m_list->setCurrentRow(row);
    const QListWidgetItem* current = m_list->item(row);
    openEntry(current == nullptr ? -1 : current->data(kEntryRole).toInt());
}

void HistoryView::openEntry(int index)
{
    askDelete(false);
    m_open = index;
    const bool has = index >= 0 && index < m_entries.size();
    m_detail->setEnabled(has);
    m_delete->setVisible(has);
    if (!has) {
        m_what->clear();
        m_meta->clear();
        m_text->clear();
        m_result->clear();
        m_diff->clear();
        return;
    }
    const history::Entry& e = m_entries.at(index);
    m_what->setText(e.label);
    QString meta = e.app.isEmpty() ? QString() : tr("from %1").arg(e.app);
    const QString when = dayName(e.when.date()) + u" "_s + timeText(e.when);
    meta = meta.isEmpty() ? when : meta + u" · "_s + when;
    m_meta->setText(u"· "_s + meta);
    m_text->setPlainText(e.text);
    // Proofread shows its changes, as the window does (note 4); the rest their text.
    const bool proofread = baseOf(e.skillId) == u"proofread"_s;
    m_result->setVisible(!proofread);
    m_diff->setVisible(proofread);
    if (proofread) {
        m_diff->showProofread(e.text, e.result);
    } else {
        m_result->setPlainText(e.result);
    }
}

void HistoryView::copyResult()
{
    if (m_open >= 0 && m_open < m_entries.size()) {
        // With a change undone in the diff, what it shows.
        QApplication::clipboard()->setText(m_diff->isHidden() ? m_entries.at(m_open).result
                                                              : m_diff->result());
    }
}

void HistoryView::askDelete(bool asking)
{
    m_confirm->setVisible(asking);
    m_delete->setVisible(!asking);
}

void HistoryView::keyPressEvent(QKeyEvent* event)
{
    if (event->matches(QKeySequence::Copy) && !m_result->textCursor().hasSelection() &&
        !m_text->textCursor().hasSelection()) {
        copyResult();
        return;
    }
    if (event->key() == Qt::Key_C && event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
        copyResult();
        return;
    }
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && m_list->hasFocus()) {
        copyResult();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        Q_EMIT backRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}

int HistoryView::shownCount() const
{
    int n = 0;
    for (int row = 0; row < m_list->count(); ++row) {
        n += m_list->item(row)->data(kEntryRole).toInt() >= 0 ? 1 : 0;
    }
    return n;
}

QString HistoryView::openResult() const
{
    return m_open >= 0 && m_open < m_entries.size() ? m_entries.at(m_open).result : QString();
}

void HistoryView::applyTheme(const Tokens& t)
{
    m_tokens = t;
    m_lock->setPixmap(Theme::icon(u"lock"_s, t.privacy, kSmallIcon).pixmap(kSmallIcon, kSmallIcon));
    m_offIcon->setPixmap(Theme::icon(u"history"_s, t.muted, 32).pixmap(32, 32));
    setButtonIcon(m_back, u"left"_s, t.text, kSmallIcon);
    setButtonIcon(m_offBack, u"left"_s, t.text, kSmallIcon);
    setButtonIcon(m_delete, u"trash"_s, t.muted, kSmallIcon);
    setButtonIcon(m_more, u"down"_s, t.muted, kSmallIcon, true);
    static_cast<ChipButton*>(m_copy)->setLeadingIcon(Theme::icon(u"copy"_s, QColor(Qt::white), kSmallIcon));
    setButtonIcon(m_again, u"refresh"_s, t.text, kSmallIcon);
    m_searchIcon->setPixmap(Theme::icon(u"search"_s, t.muted, kSmallIcon).pixmap(kSmallIcon, kSmallIcon));
    m_diff->applyTheme(t);
    m_list->viewport()->update();
}

} // namespace paraphrase::app
