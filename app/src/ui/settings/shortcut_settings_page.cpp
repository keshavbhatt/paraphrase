#include "ui/settings/shortcut_settings_page.h"

#include "ui/common/widgets.h"

#include <QApplication>
#include <QClipboard>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <utility>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

/// A card and its column: sub-layouts go in with addLayout, which gives their widgets a parent.
std::pair<QFrame*, QVBoxLayout*> card()
{
    auto* f = new QFrame;
    f->setObjectName(u"card"_s);
    auto* layout = new QVBoxLayout(f);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(10);
    return {f, layout};
}

QLabel* pill(const QString& text, bool works)
{
    QLabel* l = makeLabel(text, works ? u"pill-ok"_s : u"pill-warn"_s);
    l->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return l;
}

QLabel* wrapped(const QString& text, const QString& role)
{
    QLabel* l = makeLabel(text, role);
    l->setWordWrap(true);
    return l;
}

} // namespace

ShortcutSettingsPage::ShortcutSettingsPage(QWidget* parent)
    : QWidget(parent)
    , m_cards(new QVBoxLayout)
{
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(18);
    column->addWidget(makeLabel(tr("Shortcut"), u"h1"_s));
    m_cards->setSpacing(18);
    column->addLayout(m_cards);
    column->addStretch();
    setSupport({});
}

void ShortcutSettingsPage::setSupport(const DesktopSupport& support)
{
    while (const QLayoutItem* item = m_cards->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    // Without a global shortcut, binding it by hand comes first (note 2).
    if (support.globalShortcut) {
        m_cards->addWidget(shortcutCard(support));
        m_cards->addWidget(byHandCard(support));
    } else {
        m_cards->addWidget(byHandCard(support));
        m_cards->addWidget(shortcutCard(support));
    }
    if (!support.steps.isEmpty()) {
        m_cards->addWidget(stepsCard(support));
    }
}

QWidget* ShortcutSettingsPage::shortcutCard(const DesktopSupport& support)
{
    const auto [c, column] = card();
    auto* head = new QHBoxLayout;
    head->addWidget(makeLabel(tr("Open Paraphrase over any app"), u"h2"_s));
    head->addStretch();
    head->addWidget(
        pill(support.globalShortcut ? tr("Works") : tr("Not on this desktop"), support.globalShortcut));
    column->addLayout(head);
    if (support.globalShortcut && !support.shortcut.isEmpty()) {
        auto* keys = new QHBoxLayout;
        keys->setSpacing(6);
        for (const QString& key : support.shortcut.split(u'+')) {
            keys->addWidget(makeLabel(key, u"kbd"_s));
        }
        keys->addStretch();
        column->addLayout(keys);
        column->addWidget(wrapped(
            tr("Select text in any app and press the shortcut. To change it, open %1 and look for "
               "Paraphrase.")
                .arg(support.changeIt.isEmpty() ? tr("your desktop's shortcut settings") : support.changeIt),
            u"small"_s));
    } else {
        column->addWidget(
            wrapped(tr("This desktop does not give apps a global shortcut. Bind the command below "
                       "instead, or use the window: paste, pick an action, copy."),
                    u"small"_s));
    }
    return c;
}

QWidget* ShortcutSettingsPage::byHandCard(const DesktopSupport& support)
{
    const auto [c, column] = card();
    column->addWidget(makeLabel(tr("Bind it by hand"), u"h2"_s));
    column->addWidget(wrapped(tr("Where the desktop offers no shortcut to apps, add a custom shortcut in its "
                                 "settings that runs this command:"),
                              u"small"_s));
    auto* row = new QHBoxLayout;
    QLabel* command = makeLabel(support.command, u"cmd"_s);
    command->setObjectName(u"popupCommand"_s);
    command->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QPushButton* copy = makeButton(tr("Copy"));
    copy->setObjectName(u"copyCommand"_s);
    row->addWidget(command, 1);
    row->addWidget(copy);
    column->addLayout(row);
    const QString text = support.command;
    connect(copy, &QPushButton::clicked, copy, [text] { QApplication::clipboard()->setText(text); });
    return c;
}

QWidget* ShortcutSettingsPage::stepsCard(const DesktopSupport& support)
{
    const auto [c, column] = card();
    column->addWidget(makeLabel(tr("What works on this desktop"), u"h2"_s));
    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(10);
    grid->addWidget(makeLabel(tr("STEP"), u"th"_s), 0, 0);
    grid->addWidget(makeLabel(tr("HOW"), u"th"_s), 0, 1);
    grid->addWidget(makeLabel(tr("STATUS"), u"th"_s), 0, 2);
    int row = 1;
    for (const DesktopSupport::Step& s : support.steps) {
        grid->addWidget(makeLabel(s.step), row, 0);
        grid->addWidget(wrapped(s.how, QString()), row, 1);
        grid->addWidget(pill(s.status, s.works), row, 2);
        ++row;
    }
    grid->setColumnStretch(1, 1);
    column->addLayout(grid);
    column->addWidget(
        wrapped(tr("When a step does not work here, Paraphrase says so and offers the next best "
                   "way: copy first, or paste the result yourself. The window always works."),
                u"small"_s));
    return c;
}

} // namespace paraphrase::app
