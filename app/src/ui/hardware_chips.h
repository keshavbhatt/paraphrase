#pragma once

#include <QJsonObject>
#include <QWidget>

class QHBoxLayout;

namespace paraphrase::app {

/// "This computer: 16 GB memory · 14 CPU threads · <CPU>" as pills (setup-model.html,
/// note 1), from Status().computer.
class HardwareChips : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(HardwareChips)

public:
    explicit HardwareChips(QWidget* parent = nullptr);
    ~HardwareChips() override = default;

    void setComputer(const QJsonObject& computer);

private:
    QHBoxLayout* m_row = nullptr;
};

} // namespace paraphrase::app
