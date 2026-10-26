#include "ui/setup/step_header.h"

#include "ui/common/widgets.h"
#include "ui/common/window_style.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {
constexpr int kCheckSize = 14;
} // namespace

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
        // A finished step shows a bare check in the success colour (setup-model.html li.done),
        // in the same 18 px box, so the names beside it stay put.
        QLabel* badge = m_badges.at(i);
        badge->setProperty("badge", state);
        if (state == u"done"_s) {
            badge->setText(QString());
            badge->setPixmap(
                Theme::icon(u"check"_s, Theme::current().success, kCheckSize).pixmap(kCheckSize));
        } else {
            badge->setPixmap(QPixmap());
            badge->setText(QString::number(i + 1));
        }
        repolish(m_names.at(i));
        repolish(m_badges.at(i));
    }
}

void StepHeader::setStepName(int step, const QString& name)
{
    m_names.at(step)->setText(name);
}

} // namespace paraphrase::app
