#include "ui/common/widgets.h"

#include "ui/common/theme.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <algorithm>

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

void setButtonIcon(QPushButton* button, const QString& name, const QColor& color, int size, bool after)
{
    constexpr int kExtraGap = 4; // the button's own 2 px plus this make the mocks' 6 px
    if (name.isEmpty()) {
        button->setIcon(QIcon());
        return;
    }
    button->setIcon(Theme::icon(name, color, size, after ? kExtraGap : 0, after ? 0 : kExtraGap));
    button->setIconSize(QSize(size + kExtraGap, size));
}

QVBoxLayout* pageHeading(const QString& title, const QString& intro, int maxWidth)
{
    auto* heading = new QVBoxLayout;
    heading->setSpacing(6);
    heading->addWidget(makeLabel(title, u"h1"_s));
    QLabel* text = makeLabel(intro, u"muted"_s);
    text->setWordWrap(true);
    text->setMaximumWidth(maxWidth); // the mocks' max-width: 64ch, or 640 px on Privacy
    heading->addWidget(text);
    return heading;
}

void showOnly(QStackedWidget* stack, QWidget* page)
{
    for (int i = 0; i < stack->count(); ++i) {
        QWidget* w = stack->widget(i);
        const QSizePolicy::Policy p = w == page ? QSizePolicy::Preferred : QSizePolicy::Ignored;
        w->setSizePolicy(p, p);
    }
    stack->setCurrentWidget(page);
    stack->updateGeometry();
}

void reserveWidth(QPushButton* button, const QStringList& texts, const QIcon& icon)
{
    const QString text = button->text();
    const QIcon shown = button->icon();
    if (!icon.isNull()) {
        button->setIcon(icon);
    }
    int width = 0;
    for (const QString& t : texts) {
        button->setText(t);
        width = std::max(width, button->sizeHint().width());
    }
    button->setText(text);
    button->setIcon(shown);
    button->setMinimumWidth(width);
}

SegmentedControl::SegmentedControl(const QList<Item>& items, QWidget* parent)
    : QFrame(parent)
{
    setObjectName(u"seg"_s);
    // Its own height, never the row's: beside a taller button it stays a slim switch.
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
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

namespace {
constexpr int kHeadHeight = 57; // a 36 px button, 10 px above and below, the 1 px rule
} // namespace

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
    // mock.css .pane .ph: padding 10px 14px. One fixed height whatever the head holds, so the
    // two panes line up and the body never jumps when the action changes.
    m_head->setContentsMargins(14, 10, 14, 10);
    headFrame->setFixedHeight(kHeadHeight);
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
