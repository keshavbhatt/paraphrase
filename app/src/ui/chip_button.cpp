#include "chip_button.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

QLabel* part(const QString& name)
{
    auto* l = new QLabel;
    l->setObjectName(name);
    l->setAttribute(Qt::WA_TransparentForMouseEvents); // clicks belong to the button
    return l;
}

constexpr int kIconSize = 14;

} // namespace

ChipButton::ChipButton(const QString& label, const QString& chip, ChipSide side, QWidget* parent)
    : QPushButton(parent)
    , m_layout(new QHBoxLayout(this))
    , m_icon(part(u"icon"_s))
    , m_chip(part(u"num"_s))
    , m_label(part(u"label"_s))
    , m_trailing(part(u"icon"_s))
{
    m_layout->setContentsMargins(14, 0, 14, 0);
    m_layout->setSpacing(8);
    m_layout->addWidget(m_icon, 0, Qt::AlignVCenter);
    if (side == ChipSide::Before) {
        m_layout->addWidget(m_chip, 0, Qt::AlignVCenter);
        m_layout->addWidget(m_label, 1, Qt::AlignVCenter);
    } else {
        m_layout->addWidget(m_label, 1, Qt::AlignVCenter);
        m_layout->addWidget(m_chip, 0, Qt::AlignVCenter);
    }
    m_layout->addWidget(m_trailing, 0, Qt::AlignVCenter);
    m_label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_chip->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_icon->hide();
    m_trailing->hide();
    setLabel(label);
    setChip(chip);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed); // as wide as its parts
    connect(this, &QPushButton::toggled, this, &ChipButton::restyle);
    restyle();
}

void ChipButton::setLabel(const QString& label)
{
    m_label->setText(label);
    setAccessibleName(label);
    updateGeometry();
}

QString ChipButton::label() const
{
    return m_label->text();
}

void ChipButton::setChip(const QString& chip)
{
    m_chip->setText(chip);
    m_chip->setVisible(!chip.isEmpty());
    updateGeometry();
}

void ChipButton::setLeadingIcon(const QIcon& icon)
{
    m_icon->setPixmap(icon.pixmap(kIconSize, kIconSize));
    m_icon->setVisible(!icon.isNull());
}

void ChipButton::setTrailingIcon(const QIcon& icon)
{
    m_trailing->setPixmap(icon.pixmap(kIconSize, kIconSize));
    m_trailing->setVisible(!icon.isNull());
}

void ChipButton::setWordWrap(bool wrap)
{
    m_label->setWordWrap(wrap);
    m_layout->setContentsMargins(14, 10, 14, 10);
    setSizePolicy(wrap ? QSizePolicy::Expanding : QSizePolicy::Fixed, QSizePolicy::Preferred);
}

QSize ChipButton::sizeHint() const
{
    QSize size = m_layout->sizeHint();
    size.setHeight(std::max(size.height(), minimumHeight()));
    return size;
}

QSize ChipButton::minimumSizeHint() const
{
    QSize size = m_layout->minimumSize();
    size.setHeight(std::max(size.height(), minimumHeight()));
    return size;
}

void ChipButton::restyle()
{
    // "on": the label sits on the accent (a checked action, the primary button).
    const QString kind = property("kind").toString();
    const bool on = (isChecked() && kind == u"action"_s) || kind == u"primary"_s;
    for (QLabel* l : {m_chip, m_label}) {
        l->setProperty("on", on);
        l->style()->unpolish(l);
        l->style()->polish(l);
    }
}

bool ChipButton::event(QEvent* e)
{
    if (e->type() == QEvent::DynamicPropertyChange) {
        restyle();
    }
    return QPushButton::event(e);
}

} // namespace paraphrase::app
