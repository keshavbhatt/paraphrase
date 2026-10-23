#include "ui/setup/desktop_access.h"

#include "ui/common/widgets.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kBadge = 36;
constexpr int kBadgeIcon = 18;
constexpr int kSideWidth = 230; // "Your desktop asks in its own window" on one line

void clearLayout(QLayout* layout)
{
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        } else if (QLayout* inner = item->layout()) {
            clearLayout(inner);
        }
        delete item;
    }
}

} // namespace

DesktopAccess::DesktopAccess(QWidget* parent)
    : QWidget(parent)
{
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(12);
    m_shortcut =
        buildRow(tr("Shortcut"), tr("Opens Paraphrase on the text you selected, over the app you are in."));
    m_typing = buildRow(tr("Put the result back"), QString());
    m_shortcut.card->setObjectName(u"grantShortcut"_s);
    m_typing.card->setObjectName(u"grantTyping"_s);
    col->addWidget(m_shortcut.card);
    col->addWidget(m_typing.card);
    m_allow = makeButton(tr("Allow"), u"primary"_s);
    m_allow->setObjectName(u"allowTyping"_s);
    m_pill = makeLabel(QString(), u"pill-ok"_s);
    m_note = makeLabel(QString(), u"small"_s);
    for (QWidget* w :
         {static_cast<QWidget*>(m_allow), static_cast<QWidget*>(m_pill), static_cast<QWidget*>(m_note)}) {
        m_typing.side->addWidget(w, 0, Qt::AlignRight);
    }
    connect(m_allow, &QPushButton::clicked, this, &DesktopAccess::allow);
}

DesktopAccess::Row DesktopAccess::buildRow(const QString& title, const QString& detail)
{
    Row row;
    row.card = new QFrame;
    row.card->setProperty("role", u"grant"_s);
    auto* line = new QHBoxLayout(row.card);
    line->setContentsMargins(16, 16, 16, 16);
    line->setSpacing(14);
    row.badge = new QLabel;
    row.badge->setProperty("role", u"grantBadge"_s);
    row.badge->setFixedSize(kBadge, kBadge);
    row.badge->setAlignment(Qt::AlignCenter);
    line->addWidget(row.badge, 0, Qt::AlignVCenter);
    auto* text = new QVBoxLayout;
    text->setSpacing(4); // .grant: the title and its line together, centred in the row
    text->addStretch();
    text->addWidget(makeLabel(title, u"h2"_s));
    row.detail = makeLabel(detail, u"small"_s);
    row.detail->setWordWrap(true);
    text->addWidget(row.detail);
    text->addStretch();
    line->addLayout(text, 1);
    row.side = new QVBoxLayout;
    row.side->setSpacing(6);
    row.side->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto* sideBox = new QWidget;
    sideBox->setFixedWidth(kSideWidth); // Allow, Set or Not allowed: the row keeps one width
    sideBox->setLayout(row.side);
    line->addWidget(sideBox, 0, Qt::AlignVCenter);
    return row;
}

void DesktopAccess::setShowsShortcut(bool shows) const
{
    m_shortcut.card->setVisible(shows);
}

void DesktopAccess::setSupport(const DesktopSupport& support)
{
    m_support = support;
    showShortcut();
    showTyping();
}

void DesktopAccess::showShortcut()
{
    clearLayout(m_shortcut.side);
    if (m_support.globalShortcut && !m_support.shortcut.isEmpty()) {
        auto* keys = new QHBoxLayout;
        keys->setSpacing(4);
        keys->addStretch();
        for (const QString& key : m_support.shortcut.split(u'+')) {
            keys->addWidget(makeLabel(key, u"kbd"_s));
        }
        m_shortcut.side->addLayout(keys);
        m_shortcut.side->addWidget(makeLabel(tr("✓ Set"), u"pill-ok"_s), 0, Qt::AlignRight);
        m_shortcut.detail->setText(tr("Opens Paraphrase on the text you selected, over the app you are in."));
        return;
    }
    // No shortcut from the desktop: say how to bind one by hand (settings-general-shortcut.html).
    m_shortcut.detail->setText(
        tr("This desktop does not set it for apps. Bind %1 to keys you like in %2.")
            .arg(m_support.command, m_support.changeIt.isEmpty() ? tr("its settings") : m_support.changeIt));
    m_shortcut.side->addWidget(makeLabel(tr("By hand"), u"pill-warn"_s), 0, Qt::AlignRight);
}

void DesktopAccess::showTyping() const
{
    using Typing = DesktopSupport::Typing;
    const QString how = tr("Replace and Insert type the result into the app the text came from, and only "
                           "when you press them.");
    QString detail = how;
    bool canAsk = false;
    QString note;
    QString pill;
    QString pillRole = u"pill-ok"_s;
    switch (m_support.typing) {
    case Typing::Ready:
        pill = tr("✓ Ready");
        break;
    case Typing::Allowed:
        pill = tr("✓ Allowed");
        break;
    case Typing::NotAsked:
        detail = how + u' ' + tr("Your desktop calls this \"Control input devices\".");
        canAsk = true;
        note = m_waiting ? tr("Waiting for your answer") : tr("Your desktop asks in its own window");
        break;
    case Typing::Refused:
        detail = tr("Not allowed: the result goes on the clipboard, and the popup says \"press Ctrl+V\". "
                    "You can allow it here again.");
        canAsk = true;
        pill = tr("Not allowed");
        pillRole = u"pill-warn"_s;
        break;
    case Typing::Unavailable:
        detail = tr("This desktop does not let apps type: the result goes on the clipboard, and you "
                    "press Ctrl+V.");
        pill = tr("You paste");
        pillRole = u"pill-warn"_s;
        break;
    }
    m_typing.detail->setText(detail);
    // Asked for the first time it is the main button; after a "no", a plain one (the mock, c).
    m_allow->setProperty("kind", m_support.typing == Typing::NotAsked ? u"primary"_s : QString());
    m_allow->style()->unpolish(m_allow);
    m_allow->style()->polish(m_allow);
    m_allow->setVisible(canAsk && m_support.askTyping != nullptr);
    m_allow->setEnabled(!m_waiting);
    m_pill->setText(pill);
    m_pill->setProperty("role", pillRole);
    m_pill->style()->unpolish(m_pill);
    m_pill->style()->polish(m_pill);
    m_pill->setVisible(!pill.isEmpty());
    m_note->setText(note);
    m_note->setVisible(!note.isEmpty());
}

void DesktopAccess::allow()
{
    if (!m_support.askTyping || m_waiting) {
        return;
    }
    m_waiting = true;
    showTyping(); // "Waiting for your answer" while the desktop's dialog is open
    // After this click returns: the question waits for the user in its own event loop.
    QTimer::singleShot(0, this, [this] {
        m_support.typing = m_support.askTyping();
        m_waiting = false;
        showTyping();
    });
}

void DesktopAccess::applyTheme(const Tokens& t)
{
    m_tokens = t;
    m_shortcut.badge->setPixmap(
        Theme::icon(u"keyboard"_s, t.accent, kBadgeIcon).pixmap(kBadgeIcon, kBadgeIcon));
    m_typing.badge->setPixmap(
        Theme::icon(u"text-cursor-input"_s, t.accent, kBadgeIcon).pixmap(kBadgeIcon, kBadgeIcon));
}

} // namespace paraphrase::app
