#include "ui/setup/hardware_chips.h"

#include "ui/common/widgets.h"
#include "ui/common/window_style.h"

#include <QHBoxLayout>
#include <QLabel>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

HardwareChips::HardwareChips(QWidget* parent)
    : QWidget(parent)
    , m_row(new QHBoxLayout(this))
{
    m_row->setContentsMargins(0, 0, 0, 0);
    m_row->setSpacing(8);
}

void HardwareChips::setComputer(const QJsonObject& computer)
{
    while (const QLayoutItem* item = m_row->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    QLabel* lead = makeLabel(tr("This computer:"));
    lead->setStyleSheet(u"font-size: 12px;"_s);
    m_row->addWidget(lead);
    const QStringList chips = {
        tr("%1 GB memory").arg(qRound(computer.value(u"memory_gb"_s).toDouble())),
        tr("%1 CPU threads").arg(computer.value(u"cpus"_s).toInt()),
        computer.value(u"cpu"_s).toString(),
    };
    for (const QString& chip : chips) {
        if (!chip.trimmed().isEmpty()) {
            m_row->addWidget(makeLabel(chip, u"pill"_s));
        }
    }
    m_row->addStretch();
}

} // namespace paraphrase::app
