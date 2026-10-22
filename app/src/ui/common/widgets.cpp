#include "ui/common/widgets.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

QLabel* makeLabel(const QString& text, const QString& role)
{
    auto* l = new QLabel(text);
    if (!role.isEmpty()) {
        l->setProperty("role", role);
    }
    return l;
}

QPushButton* makeButton(const QString& text, const QString& kind)
{
    auto* b = new QPushButton(text);
    if (!kind.isEmpty()) {
        b->setProperty("kind", kind);
    }
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

SegmentedControl::SegmentedControl(const QList<Item>& items, QWidget* parent)
    : QFrame(parent)
{
    setObjectName(u"seg"_s);
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(2, 2, 2, 2);
    row->setSpacing(0);
    auto* group = new QButtonGroup(this);
    for (const Item& item : items) {
        QPushButton* b = makeButton(item.text, u"segment"_s);
        b->setObjectName(item.name);
        b->setCheckable(true);
        group->addButton(b, static_cast<int>(m_buttons.size()));
        row->addWidget(b);
        m_buttons.append(b);
    }
    if (!m_buttons.isEmpty()) {
        m_buttons.front()->setChecked(true);
    }
    connect(group, &QButtonGroup::idClicked, this, &SegmentedControl::changed);
}

int SegmentedControl::current() const
{
    for (qsizetype i = 0; i < m_buttons.size(); ++i) {
        if (m_buttons.at(i)->isChecked()) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void SegmentedControl::setCurrent(int index)
{
    if (index >= 0 && index < m_buttons.size()) {
        m_buttons.at(index)->setChecked(true);
    }
}

Pane::Pane(QWidget* parent)
    : QFrame(parent)
    , m_layout(new QVBoxLayout(this))
{
    setObjectName(u"pane"_s);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    auto* headFrame = new QFrame;
    headFrame->setObjectName(u"paneHead"_s);
    m_head = new QHBoxLayout(headFrame);
    m_head->setContentsMargins(14, 8, 10, 8);
    m_head->setSpacing(8);
    m_layout->addWidget(headFrame);
}

void Pane::setBody(QWidget* body)
{
    m_layout->insertWidget(1, body, 1);
}

QHBoxLayout* Pane::footRow()
{
    if (m_foot == nullptr) {
        m_foot = new QFrame;
        m_foot->setObjectName(u"paneFoot"_s);
        m_footRow = new QHBoxLayout(m_foot);
        m_footRow->setContentsMargins(14, 10, 14, 10);
        m_footRow->setSpacing(8);
        m_layout->addWidget(m_foot);
    }
    return m_footRow;
}

} // namespace paraphrase::app
