#pragma once

#include <QFrame>
#include <QList>
#include <QStringList>

class QLabel;

namespace paraphrase::app {

/// The setup window's header band (setup-model.html): the title and the numbered steps, the
/// current one filled, the finished ones ticked.
class StepHeader : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(StepHeader)

public:
    StepHeader(const QString& title, const QStringList& steps, QWidget* parent = nullptr);
    ~StepHeader() override = default;

    /// Marks `current` as the step in progress and the earlier ones in `done` as finished.
    void setStep(int current, const QList<int>& done);
    void setStepName(int step, const QString& name);

private:
    QList<QLabel*> m_names;
    QList<QLabel*> m_badges;
};

} // namespace paraphrase::app
