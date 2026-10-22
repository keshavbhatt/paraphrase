#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

namespace paraphrase::app {

class SampleRun;

/// The setup window's last step: proofread one sentence with the model just installed.
class TryPage : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TryPage)

public:
    explicit TryPage(SampleRun& run, QWidget* parent = nullptr);
    ~TryPage() override = default;

private:
    void tryIt();
    void showAnswer(const QString& text, const QString& role);

    SampleRun& m_run;
    QLineEdit* m_sample = nullptr;
    QPushButton* m_button = nullptr;
    QLabel* m_answer = nullptr;
};

} // namespace paraphrase::app
