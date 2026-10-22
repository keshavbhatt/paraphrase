#include "ui/setup/step_header.h"

#include "ui/common/widgets.h"
#include "ui/common/window_style.h"

#include <QHBoxLayout>
#include <QLabel>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

StepHeader::StepHeader(const QString& title, const QStringList& steps, QWidget* parent)
    : QFrame(parent)
{
    setObjectName(u"band"_s);
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(24, 16, 24, 16);
    row->setSpacing(18);
    row->addWidget(makeLabel(title, u"title"_s));
    row->addStretch();
    for (int i = 0; i < steps.size(); ++i) {
        auto* step = new QHBoxLayout;
        step->setSpacing(6);
        auto* badge = new QLabel(QString::number(i + 1));
        badge->setFixedSize(18, 18);
        badge->setAlignment(Qt::AlignCenter);
        auto* name = new QLabel(steps.at(i));
        name->setObjectName(u"step%1"_s.arg(i));
        step->addWidget(badge);
        step->addWidget(name);
        row->addLayout(step);
        m_badges.append(badge);
        m_names.append(name);
    }
}

void StepHeader::setStep(int current, const QList<int>& done)
{
    for (int i = 0; i < m_names.size(); ++i) {
        QString state = u"todo"_s;
        if (i == current) {
            state = u"on"_s;
        } else if (done.contains(i)) {
            state = u"done"_s;
        }
        m_names.at(i)->setProperty("step", state);
        m_badges.at(i)->setProperty("badge", state == u"on"_s ? u"on"_s : u"todo"_s);
        m_badges.at(i)->setText(state == u"done"_s ? u"✓"_s : QString::number(i + 1));
        repolish(m_names.at(i));
        repolish(m_badges.at(i));
    }
}

void StepHeader::setStepName(int step, const QString& name)
{
    m_names.at(step)->setText(name);
}

} // namespace paraphrase::app
