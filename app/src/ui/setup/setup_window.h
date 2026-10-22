#pragma once

#include "ui/common/theme.h"

#include <QWidget>

class QLabel;
class QPushButton;
class QStackedWidget;

namespace paraphrase::client {
class DaemonClient;
}

namespace paraphrase::app {

class ChipButton;
class ModelPage;
class ModelsController;
class SampleRun;
class StepHeader;

/// First run (setup-model.html), shown when no model is installed: the shortcut step (not
/// built yet, so it says what is coming), choosing and downloading a model, then trying it.
class SetupWindow : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(SetupWindow)

public:
    enum Step : std::uint8_t
    {
        Shortcut,
        Model,
        Try,
    };

    explicit SetupWindow(client::DaemonClient& client, QWidget* parent = nullptr);
    ~SetupWindow() override;

    /// Reads the computer and the models and shows the model step.
    void start();
    [[nodiscard]] Step step() const;

Q_SIGNALS:
    void finished(); ///< the user is done: open the main window

protected:
    void changeEvent(QEvent* event) override;

private:
    [[nodiscard]] QWidget* buildFooter();
    void showStep(int step);
    void updateFooter();
    void applyTheme();

    Tokens m_tokens;
    ModelsController* m_models;
    SampleRun* m_run;
    StepHeader* m_header;
    QStackedWidget* m_pages;
    ModelPage* m_modelPage;
    QPushButton* m_back = nullptr;
    QLabel* m_footNote = nullptr;
    ChipButton* m_next = nullptr;
};

} // namespace paraphrase::app
