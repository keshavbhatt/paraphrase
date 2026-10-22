#include "settings_nav.h"

#include "widgets.h"
#include "window_style.h"

#include <QButtonGroup>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

SettingsNav::SettingsNav(const QStringList& pages, QWidget* parent)
    : QWidget(parent)
    , m_group(new QButtonGroup(this))
{
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(2);
    for (int i = 0; i < pages.size(); ++i) {
        QPushButton* b = makeButton(pages.at(i), u"nav"_s);
        b->setObjectName(u"nav%1"_s.arg(i));
        b->setCheckable(true);
        m_group->addButton(b, i);
        col->addWidget(b);
    }
    col->addStretch();
    setFixedWidth(190);
    connect(m_group, &QButtonGroup::idClicked, this, &SettingsNav::pageChosen);
}

void SettingsNav::setCurrent(int page)
{
    if (QAbstractButton* b = m_group->button(page)) {
        b->setChecked(true);
    }
}

} // namespace paraphrase::app
