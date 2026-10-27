#include "ui/settings/shortcut_settings_page.h"

#include "ui/common/elided_label.h"
#include "ui/common/widgets.h"
#include "ui/core/preferences.h"
#include "ui/setup/desktop_access.h"

#include <QApplication>
#include <QButtonGroup>
#include <QClipboard>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QRadioButton>
#include <QStyle>
#include <QVBoxLayout>

#include <utility>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

/// An option as a card: a click anywhere on it picks it, not only on the small circle.
class OptionCard : public QFrame
{
public:
    QRadioButton* radio = nullptr;

protected:
    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint())) {
            radio->setChecked(true);
            radio->setFocus(Qt::MouseFocusReason);
        }
        QFrame::mouseReleaseEvent(event);
    }
};

/// A card and its column: sub-layouts go in with addLayout, which gives their widgets a parent.
std::pair<QFrame*, QVBoxLayout*> card()
{
    auto* f = new QFrame;
    f->setObjectName(u"card"_s);
    auto* layout = new QVBoxLayout(f);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(12);
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
    , m_access(new DesktopAccess)
{
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(18);
    column->addWidget(makeLabel(tr("Shortcut"), u"h1"_s));
    // "Put the result back" as in setup's first step (setup-desktop.html, note 3); the
    // shortcut has its own cards below.
    m_access->setShowsShortcut(false);
    column->addWidget(m_access);
    column->addWidget(opensCard());
    m_cards->setSpacing(18);
    column->addLayout(m_cards);
    column->addStretch();
    setSupport({});
}

QWidget* ShortcutSettingsPage::opensCard()
{
    // popup-working.html, d: what the shortcut opens; saved at once.
    auto* card = new QFrame;
    card->setObjectName(u"card"_s);
    auto* col = new QVBoxLayout(card);
    col->setContentsMargins(18, 16, 18, 16);
    col->setSpacing(12);
    col->addWidget(makeLabel(tr("The shortcut opens"), u"h2"_s));
    const bool window = Preferences::shortcutOpensWindow();
    auto* group = new QButtonGroup(card);
    const auto option = [&](const QString& name, const QString& title, const QString& detail, bool on) {
        auto* box = new OptionCard;
        box->setCursor(Qt::PointingHandCursor);
        box->setProperty("role", u"opt"_s);
        box->setProperty("on", on);
        auto* row = new QHBoxLayout(box);
        row->setContentsMargins(14, 12, 14, 12);
        row->setSpacing(10);
        auto* radio = new QRadioButton;
        box->radio = radio;
        radio->setObjectName(name);
        radio->setChecked(on);
        radio->setAccessibleName(title);
        group->addButton(radio);
        row->addWidget(radio, 0, Qt::AlignTop);
        auto* text = new QVBoxLayout;
        text->setSpacing(2);
        QLabel* head = makeLabel(u"<b>%1</b>"_s.arg(title.toHtmlEscaped()));
        head->setBuddy(radio);
        text->addWidget(head);
        QLabel* more = makeLabel(detail, u"small"_s);
        more->setWordWrap(true);
        text->addWidget(more);
        row->addLayout(text, 1);
        // The labels pass clicks through to the card.
        head->setAttribute(Qt::WA_TransparentForMouseEvents);
        more->setAttribute(Qt::WA_TransparentForMouseEvents);
        connect(radio, &QRadioButton::toggled, box, [box](bool checked) {
            box->setProperty("on", checked);
            box->style()->unpolish(box);
            box->style()->polish(box);
        });
        col->addWidget(box);
        return radio;
    };
    option(u"opensPopup"_s, tr("The popup, over the app you are in"),
           tr("Pick an action and the result goes back where the text was. Recommended."), !window);
    const QRadioButton* toWindow = option(
        u"opensWindow"_s, tr("Paraphrase's window"),
        tr("The selected text opens in the main window, ready for any action. Copy the result yourself."),
        window);
    connect(toWindow, &QRadioButton::toggled, this, [](bool on) { Preferences::setShortcutOpensWindow(on); });
    return card;
}

void ShortcutSettingsPage::applyTheme(const Tokens& t)
{
    m_access->applyTheme(t);
}

void ShortcutSettingsPage::setSupport(const DesktopSupport& support)
{
    m_access->setSupport(support);
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
    const bool hasKeys = support.globalShortcut && !support.shortcut.isEmpty();
    const bool noKeys = support.globalShortcut && !hasKeys && support.chooseKeys;
    auto* head = new QHBoxLayout;
    head->addWidget(makeLabel(tr("Open Paraphrase over any app"), u"h2"_s));
    head->addStretch();
    if (noKeys) {
        head->addWidget(pill(tr("No keys"), false));
    } else {
        head->addWidget(
            pill(support.globalShortcut ? tr("Works") : tr("Not on this desktop"), support.globalShortcut));
    }
    column->addLayout(head);
    const std::function<void()> choose = support.chooseKeys;
    const auto chooseButton = [&choose](const QString& label, const QString& kind, const QString& name) {
        QPushButton* b = makeButton(label, kind);
        b->setObjectName(name);
        QObject::connect(b, &QPushButton::clicked, b, [choose] { choose(); });
        return b;
    };
    if (hasKeys) {
        // settings-general-shortcut.html: the keys, and Change keys opens the desktop's window.
        auto* keys = new QHBoxLayout;
        keys->setSpacing(6);
        for (const QString& key : support.shortcut.split(u'+')) {
            keys->addWidget(makeLabel(key, u"kbd"_s));
        }
        keys->addStretch();
        const bool canChange = support.canChangeKeys && support.chooseKeys;
        if (canChange) {
            keys->addWidget(chooseButton(tr("Change keys"), QString(), u"changeKeys"_s));
        }
        column->addLayout(keys);
        column->addWidget(wrapped(
            canChange
                ? tr("Select text in any app and press the shortcut.")
                : tr("Select text in any app and press the shortcut. To change it, open %1 and look for "
                     "Paraphrase.")
                      .arg(support.changeIt.isEmpty() ? tr("your desktop's shortcut settings")
                                                      : support.changeIt),
            u"small"_s));
    } else if (noKeys) {
        // The desktop offers app shortcuts, but this one has none: taken by another app.
        auto* row = new QHBoxLayout;
        row->addWidget(
            wrapped(tr("Your desktop has the shortcut but gave it no keys, for example because another "
                       "app already uses Ctrl+Alt+Space."),
                    u"small"_s),
            1);
        row->addWidget(chooseButton(tr("Choose keys"), u"primary"_s, u"chooseKeys"_s), 0, Qt::AlignVCenter);
        column->addLayout(row);
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
    auto* command = new ElidedLabel(support.command);
    command->setProperty("role", u"cmd"_s);
    command->setObjectName(u"popupCommand"_s);
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
    // settings-general-shortcut.html table.list: th 8px 10px, td 11px 10px, a rule under each row.
    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(0);
    grid->setVerticalSpacing(0);
    const auto cell = [](QWidget* w, int vertical) {
        w->setContentsMargins(10, vertical, 10, vertical);
        return w;
    };
    const auto rule = [grid](int row) {
        auto* line = new QFrame;
        line->setObjectName(u"rule"_s);
        grid->addWidget(line, row, 0, 1, 3);
    };
    grid->addWidget(cell(makeLabel(tr("STEP"), u"th"_s), 8), 0, 0);
    grid->addWidget(cell(makeLabel(tr("HOW"), u"th"_s), 8), 0, 1);
    grid->addWidget(cell(makeLabel(tr("STATUS"), u"th"_s), 8), 0, 2);
    rule(1);
    int row = 2;
    for (const DesktopSupport::Step& s : support.steps) {
        grid->addWidget(cell(makeLabel(s.step), 11), row, 0, Qt::AlignTop);
        grid->addWidget(cell(wrapped(s.how, QString()), 11), row, 1, Qt::AlignTop);
        auto* status = new QWidget;
        auto* statusRow = new QHBoxLayout(status);
        statusRow->setContentsMargins(10, 10, 10, 10);
        statusRow->addWidget(pill(s.status, s.works));
        grid->addWidget(status, row, 2, Qt::AlignTop | Qt::AlignLeft);
        if (&s != &support.steps.last()) {
            rule(row + 1);
        }
        row += 2;
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
