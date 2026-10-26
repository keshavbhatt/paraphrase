#include "ui/settings/privacy_page.h"

#include "history/history_keeper.h"
#include "ui/common/flow_layout.h"
#include "ui/common/toggle_switch.h"
#include "ui/common/widgets.h"
#include "ui/common/window_style.h"
#include "ui/models/model_info.h"

#include <QComboBox>
#include <QDate>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>

#include <array>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr std::array<int, 4> kKeepDays = {7, 30, 365, 0};
constexpr int kIntroWidth = 640; // settings-privacy.html

/// One `.opt` row: a bold title, a small detail, and the control on the right.
QFrame* optionRow(const QString& title, QLabel* detail, QWidget* control, QWidget* below = nullptr)
{
    auto* row = new QFrame;
    row->setObjectName(u"actionRow"_s); // the same divided rows as General
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 12, 0, 12);
    layout->setSpacing(12);
    auto* what = new QVBoxLayout;
    what->setSpacing(2);
    what->addWidget(makeLabel(u"<b>%1</b>"_s.arg(title.toHtmlEscaped())));
    detail->setWordWrap(true);
    what->addWidget(detail);
    if (below != nullptr) {
        below->setContentsMargins(0, 6, 0, 0); // a gap that hides with it
        what->addWidget(below);
    }
    layout->addLayout(what, 1);
    if (control != nullptr) {
        layout->addWidget(control, 0, Qt::AlignTop);
    }
    return row;
}

QString problemText(history::Keeper::Problem problem)
{
    switch (problem) {
    case history::Keeper::Problem::NoKeyring:
        return PrivacyPage::tr(
            "History needs your desktop's keyring to keep its key (KWallet, GNOME Keyring or "
            "another Secret Service). None answered, or it was not unlocked.");
    case history::Keeper::Problem::WrongKey:
        return PrivacyPage::tr("The history kept earlier was locked with a key your keyring no longer has. "
                               "Delete all history below to start a new one.");
    case history::Keeper::Problem::Failed:
    case history::Keeper::Problem::None:
        break;
    }
    return PrivacyPage::tr("The history file could not be opened or made.");
}

QString oldestText(const QDateTime& when)
{
    const QDate day = when.date();
    const QString format = day.year() == QDate::currentDate().year() ? u"d MMMM"_s : u"d MMMM yyyy"_s;
    return QLocale().toString(day, format);
}

} // namespace

PrivacyPage::PrivacyPage(history::Keeper& keeper, QWidget* parent)
    : QWidget(parent)
    , m_keeper(keeper)
    , m_switch(new ToggleSwitch)
    , m_onRows(new QWidget)
    , m_keepFor(new QComboBox)
    , m_apps(new QWidget)
    , m_appsFlow(new FlowLayout(m_apps, 8))
    , m_appName(new QLineEdit)
    , m_factsRow(new QFrame)
    , m_lock(new QLabel)
    , m_confirm(new QFrame)
{
    m_switch->setObjectName(u"keepHistory"_s);
    m_switch->setAccessibleName(tr("Keep a history"));
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(18);
    column->addLayout(pageHeading(
        tr("Privacy"),
        tr("What you write is read by the model on this computer and nowhere else. There is no "
           "account and no usage data. Paraphrase goes online only to download a model you chose."),
        kIntroWidth));

    auto* card = new QFrame;
    card->setObjectName(u"card"_s);
    auto* rows = new QVBoxLayout(card);
    rows->setContentsMargins(18, 4, 18, 4);
    rows->setSpacing(0);

    // 1. The switch, and why it cannot turn on.
    m_switchDetail = makeLabel({}, u"small"_s);
    m_problem = makeLabel({}, u"small"_s);
    m_problem->setObjectName(u"historyProblem"_s);
    m_problem->setWordWrap(true);
    m_problem->hide();
    m_switchRow = optionRow(tr("Keep a history"), m_switchDetail, m_switch, m_problem);
    rows->addWidget(m_switchRow);

    // 2 to 5, only while it is on.
    auto* onRows = new QVBoxLayout(m_onRows);
    onRows->setContentsMargins(0, 0, 0, 0);
    onRows->setSpacing(0);

    addOptions(onRows);
    buildFacts();
    onRows->addWidget(m_factsRow);
    rows->addWidget(m_onRows);

    // 6. Asked on the page: Delete all, or after turning off.
    rows->addWidget(buildConfirm());
    column->addWidget(card);
    column->addStretch();

    wire();
    reload();
}

void PrivacyPage::addOptions(QVBoxLayout* onRows)
{
    m_keepFor->setObjectName(u"keepFor"_s);
    m_keepFor->setAccessibleName(tr("Keep entries for"));
    m_keepFor->addItems({tr("7 days"), tr("30 days"), tr("1 year"), tr("Until I delete them")});
    onRows->addWidget(optionRow(tr("Keep entries for"),
                                makeLabel(tr("Older entries are deleted on their own."), u"small"_s),
                                m_keepFor));

    m_appsFlow->setContentsMargins(0, 0, 0, 0);
    m_addApp = makeButton(tr("Add an app"), u"flat"_s);
    m_addApp->setObjectName(u"addApp"_s);

    m_appName->setObjectName(u"appName"_s);
    m_appName->setPlaceholderText(tr("App name, then Enter"));
    m_appName->setAccessibleName(tr("App to leave out"));
    m_appName->hide();
    onRows->addWidget(
        optionRow(tr("Not from these apps"),
                  makeLabel(tr("Text from these apps is never kept, even with history on."), u"small"_s),
                  nullptr, m_apps));
}

void PrivacyPage::wire()
{
    connect(m_switch, &QAbstractButton::toggled, this, &PrivacyPage::switched);
    connect(m_keepFor, &QComboBox::activated, this, [this](int index) {
        m_keeper.setKeepDays(kKeepDays.at(static_cast<std::size_t>(std::clamp(index, 0, 3))));
    });
    connect(m_addApp, &QPushButton::clicked, this, [this] {
        m_appName->show();
        m_appName->setFocus();
    });
    connect(m_appName, &QLineEdit::returnPressed, this, &PrivacyPage::addApp);
    connect(m_deleteAll, &QPushButton::clicked, this, [this] { ask(Asking::DeleteAll); });
    connect(&m_keeper, &history::Keeper::changed, this, &PrivacyPage::reload);
}

void PrivacyPage::buildFacts()
{
    m_factsRow->setObjectName(u"actionRow"_s);
    m_factsRow->setProperty("last", true);
    auto* factsLayout = new QHBoxLayout(m_factsRow);
    factsLayout->setContentsMargins(0, 12, 0, 12);
    factsLayout->setSpacing(12);
    auto* facts = new QGridLayout;
    facts->setHorizontalSpacing(16);
    facts->setVerticalSpacing(6);
    m_kept = makeLabel({});
    m_kept->setObjectName(u"historyKept"_s);

    m_where = makeLabel(tr("Encrypted on this computer, with a key in your system's keyring"));
    m_where->setWordWrap(true);
    m_size = makeLabel({});
    auto* where = new QHBoxLayout;
    where->setSpacing(6);
    m_lock->setContentsMargins(0, 3, 0, 0); // on the first line's centre
    where->addWidget(m_lock, 0, Qt::AlignTop);
    where->addWidget(m_where, 1);
    facts->addWidget(makeLabel(tr("Kept"), u"muted"_s), 0, 0);
    facts->addWidget(m_kept, 0, 1);
    facts->addWidget(makeLabel(tr("Where"), u"muted"_s), 1, 0, Qt::AlignTop);
    facts->addLayout(where, 1, 1);
    facts->addWidget(makeLabel(tr("Size"), u"muted"_s), 2, 0);
    facts->addWidget(m_size, 2, 1);
    facts->setColumnStretch(1, 1);
    factsLayout->addLayout(facts, 1);
    m_deleteAll = makeButton(tr("Delete all history"), u"dangerOutline"_s);
    m_deleteAll->setObjectName(u"deleteAllHistory"_s);
    factsLayout->addWidget(m_deleteAll, 0, Qt::AlignTop);
}

QWidget* PrivacyPage::buildConfirm()
{
    m_confirm->setObjectName(u"confirm"_s);
    m_confirm->setAccessibleName(tr("Delete all history"));
    auto* confirm = new QHBoxLayout(m_confirm);
    confirm->setContentsMargins(14, 12, 14, 12);
    confirm->setSpacing(10);
    auto* question = new QVBoxLayout;
    question->setSpacing(2);
    m_question = makeLabel({}, u"strong"_s);
    m_questionDetail = makeLabel({}, u"small"_s);
    m_questionDetail->setWordWrap(true);
    question->addWidget(m_question);
    question->addWidget(m_questionDetail);
    confirm->addLayout(question, 1);
    QPushButton* keep = makeButton(tr("Keep them"));
    keep->setObjectName(u"keepThem"_s);
    QPushButton* remove = makeButton(tr("Delete all"), u"dangerSolid"_s);
    remove->setObjectName(u"confirmDeleteAll"_s);
    confirm->addWidget(keep);
    confirm->addWidget(remove);
    m_confirm->hide();
    auto* confirmWrap = new QWidget;
    confirmWrap->hide(); // shown with the question: hidden, its padding takes no room
    auto* confirmPad = new QVBoxLayout(confirmWrap);
    confirmPad->setContentsMargins(0, 4, 0, 12);
    confirmPad->addWidget(m_confirm);
    connect(keep, &QPushButton::clicked, this, [this] { ask(Asking::Nothing); });
    connect(remove, &QPushButton::clicked, this, [this] {
        // The second, explicit click.
        m_keeper.deleteAll();
        ask(Asking::Nothing);
    });
    return confirmWrap;
}

void PrivacyPage::switched(bool on)
{
    m_problem->hide();
    if (on) {
        const history::Keeper::Problem problem = m_keeper.turnOn();
        if (problem != history::Keeper::Problem::None) {
            const QSignalBlocker block(m_switch);
            m_switch->setChecked(false);
            m_problem->setText(problemText(problem));
            m_problem->show();
            if (problem == history::Keeper::Problem::WrongKey) {
                m_asking = Asking::AfterOff;
                ask(Asking::AfterOff);
            }
            return;
        }
        ask(Asking::Nothing);
        return;
    }
    const int kept = m_keeper.stats().count;
    m_keeper.turnOff(false);
    ask(kept > 0 ? Asking::AfterOff : Asking::Nothing);
}

void PrivacyPage::ask(Asking what)
{
    m_asking = what;
    if (what != Asking::Nothing) {
        const int count = m_keeper.stats().count;
        if (what == Asking::DeleteAll) {
            m_question->setText(count == 1 ? tr("Delete the one entry?")
                                           : tr("Delete all %1 entries?").arg(count));
        } else if (count > 0) {
            m_question->setText(count == 1 ? tr("Delete the entry kept so far?")
                                           : tr("Delete the %1 entries kept so far?").arg(count));
        } else {
            m_question->setText(tr("Delete the history kept earlier?"));
        }
        m_questionDetail->setText(what == Asking::DeleteAll
                                      ? tr("This cannot be undone. History stays on for what comes next.")
                                      : tr("This cannot be undone. Keep them to see them again when history "
                                           "is back on."));
    }
    m_confirm->setVisible(what != Asking::Nothing);
    m_confirm->parentWidget()->setVisible(what != Asking::Nothing); // and its padding
    m_deleteAll->setEnabled(what == Asking::Nothing);
}

void PrivacyPage::addApp()
{
    const QString app = m_appName->text().simplified();
    m_appName->clear();
    m_appName->hide();
    QStringList apps = history::Keeper::excludedApps();
    if (!app.isEmpty() && !apps.contains(app, Qt::CaseInsensitive)) {
        apps.append(app);
        m_keeper.setExcludedApps(apps);
    }
}

void PrivacyPage::showApps()
{
    while (const QLayoutItem* item = m_appsFlow->takeAt(0)) {
        if (item->widget() != m_addApp && item->widget() != m_appName) {
            delete item->widget();
        }
        delete item;
    }
    for (const QString& app : history::Keeper::excludedApps()) {
        QPushButton* pill = makeButton(app + u"  ✕"_s, u"pill"_s);
        pill->setObjectName(u"excluded-"_s + app);
        pill->setAccessibleName(tr("Keep text from %1 again").arg(app));
        connect(pill, &QPushButton::clicked, this, [this, app] {
            QStringList apps = history::Keeper::excludedApps();
            apps.removeAll(app);
            m_keeper.setExcludedApps(apps);
        });
        m_appsFlow->addWidget(pill);
    }
    m_appsFlow->addWidget(m_addApp);
    m_appsFlow->addWidget(m_appName);
}

void PrivacyPage::reload()
{
    const bool on = history::Keeper::isOn();
    {
        const QSignalBlocker block(m_switch);
        m_switch->setChecked(on);
    }
    m_switchDetail->setText(on ? tr("Keep what you worked on and the results, so you can copy or reuse them "
                                    "later. Open it from History in the main window.")
                               : tr("Keep what you worked on and the results, so you can copy or reuse them "
                                    "later. Off: nothing you write is kept after you close it."));
    m_onRows->setVisible(on);
    // Alone, the switch row is the card's last and draws no rule under it.
    m_switchRow->setProperty("last", !on);
    repolish(m_switchRow);
    if (on) {
        m_problem->hide();
        const auto* it = std::ranges::find(kKeepDays, history::Keeper::keepDays());
        m_keepFor->setCurrentIndex(it == kKeepDays.end() ? 1 : static_cast<int>(it - kKeepDays.begin()));
        showApps();
        const history::Stats stats = m_keeper.stats();
        if (stats.count == 0) {
            m_kept->setText(tr("Nothing yet"));
        } else if (stats.count == 1) {
            m_kept->setText(tr("1 entry, from %1").arg(oldestText(stats.oldest)));
        } else {
            m_kept->setText(
                tr("%1 entries, the oldest from %2").arg(stats.count).arg(oldestText(stats.oldest)));
        }
        m_size->setText(formatSize(stats.bytes));
    }
    if (m_asking != Asking::Nothing) {
        ask(m_asking);
    }
}

void PrivacyPage::applyTheme(const Tokens& t)
{
    m_tokens = t;
    m_switch->applyTheme(t);
    m_lock->setPixmap(Theme::icon(u"lock"_s, t.privacy, 14).pixmap(14, 14));
    setButtonIcon(m_addApp, u"plus"_s, t.text, 14);
    setButtonIcon(m_deleteAll, u"trash"_s, t.danger, 14);
    m_problem->setStyleSheet(u"color: %1"_s.arg(t.danger.name()));
}

} // namespace paraphrase::app
