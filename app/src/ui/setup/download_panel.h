#pragma once

#include "ui/models/model_info.h"

#include <QFrame>
#include <QString>

class QLabel;
class QProgressBar;
class QPushButton;

namespace paraphrase::app {

/// One model's download (setup-model.html, note 4): its state in words, the progress, one
/// button (Download, Pause, Resume or Retry) and the exact source so it can be checked.
class DownloadPanel : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(DownloadPanel)

public:
    explicit DownloadPanel(const ModelInfo& model, QWidget* parent = nullptr);
    ~DownloadPanel() override = default;

    /// Shows the model's download state; `speed` in bytes per second, 0 when unknown.
    void update(const ModelInfo& model, double speed);

Q_SIGNALS:
    /// The button was clicked: download, pause, resume or retry this model.
    void actionClicked(const QString& modelId);

private:
    QString m_modelId;
    QLabel* m_title = nullptr;
    QLabel* m_stats = nullptr;
    QPushButton* m_action = nullptr;
    QProgressBar* m_bar = nullptr;
};

} // namespace paraphrase::app
