#include "ui/setup/shortcut_page.h"

#include "ui/common/widgets.h"
#include "ui/setup/desktop_access.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

ShortcutPage::ShortcutPage(QWidget* parent)
    : QWidget(parent)
    , m_access(new DesktopAccess)
    , m_lock(new QLabel)
{
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(24, 22, 24, 22);
    col->setSpacing(16);
    auto* intro = new QVBoxLayout;
    intro->setSpacing(8);
    intro->addWidget(makeLabel(tr("Use Paraphrase over any app"), u"h1"_s));
    QLabel* text =
        makeLabel(tr("Select text anywhere, press the shortcut, pick an action, and the result goes "
                     "back where the text was. Your desktop asks you about each of these once. You "
                     "can skip both: everything also works in Paraphrase's own window."),
                  u"muted"_s);
    text->setWordWrap(true);
    text->setMaximumWidth(620);
    intro->addWidget(text);
    col->addLayout(intro);
    col->addWidget(m_access);
    auto* privacy = new QHBoxLayout;
    privacy->setSpacing(10);
    privacy->addWidget(m_lock, 0, Qt::AlignTop);
    QLabel* never =
        makeLabel(tr("Paraphrase types only after you press Replace or Insert, into the window the "
                     "text came from, and never sends a message. Nothing you write leaves this "
                     "computer."),
                  u"small"_s);
    never->setWordWrap(true);
    privacy->addWidget(never, 1);
    col->addLayout(privacy);
    col->addStretch();
}

void ShortcutPage::setSupport(const DesktopSupport& support)
{
    m_access->setSupport(support);
}

void ShortcutPage::applyTheme(const Tokens& t)
{
    m_access->applyTheme(t);
    m_lock->setPixmap(Theme::icon(u"lock"_s, t.privacy, 14).pixmap(14, 14));
}

} // namespace paraphrase::app
