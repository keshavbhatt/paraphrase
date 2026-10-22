#include "download_panel.h"

#include "widgets.h"
#include "window_style.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

#include <cmath>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kSecondsPerMinute = 60;
constexpr int kBarSteps = 1000;

/// What the panel says for each download state.
struct View
{
    QString title;
    QString stats;
    QString action; ///< "" hides the button
};

qint64 totalOf(const ModelInfo& m)
{
    return m.total > 0 ? m.total : m.size;
}

QString timeLeft(const ModelInfo& m, double speed)
{
    if (speed <= 0) {
        return {};
    }
    const auto left = static_cast<qint64>(std::ceil(static_cast<double>(totalOf(m) - m.received) / speed));
    const QString text = DownloadPanel::tr(" · %1/s").arg(formatSize(static_cast<qint64>(speed)));
    if (left < kSecondsPerMinute) {
        return text + DownloadPanel::tr(" · under a minute left");
    }
    return text + DownloadPanel::tr(" · about %1 min left")
                      .arg((left + (kSecondsPerMinute / 2)) / kSecondsPerMinute);
}

View view(const QString& title, const QString& stats, const QString& action = {})
{
    return {.title = title, .stats = stats, .action = action};
}

View viewOf(const ModelInfo& m, double speed)
{
    const QString& state = m.downloadState;
    const QString progress =
        DownloadPanel::tr("%1 of %2").arg(formatSize(m.received), formatSize(totalOf(m)));
    if (m.installed) {
        return view(DownloadPanel::tr("%1 is ready").arg(m.name),
                    DownloadPanel::tr("Checked: the fingerprint matches"));
    }
    if (state == u"downloading"_s || state == u"waiting"_s) {
        return view(DownloadPanel::tr("Downloading %1").arg(m.name), progress + timeLeft(m, speed),
                    DownloadPanel::tr("Pause"));
    }
    if (state == u"verifying"_s) {
        return view(DownloadPanel::tr("Checking %1").arg(m.name),
                    DownloadPanel::tr("Comparing its SHA-256 fingerprint"));
    }
    if (state == u"paused"_s) {
        return view(DownloadPanel::tr("%1 is paused").arg(m.name), progress, DownloadPanel::tr("Resume"));
    }
    if (state == u"failed"_s) {
        return view(DownloadPanel::tr("%1 did not download").arg(m.name), m.downloadError,
                    DownloadPanel::tr("Retry"));
    }
    return view(DownloadPanel::tr("Download %1").arg(m.name), formatSize(m.size),
                DownloadPanel::tr("Download"));
}

} // namespace

DownloadPanel::DownloadPanel(const ModelInfo& model, QWidget* parent)
    : QFrame(parent)
    , m_modelId(model.id)
    , m_title(makeLabel(QString()))
    , m_stats(makeLabel(QString()))
    , m_action(makeButton(QString()))
    , m_bar(new QProgressBar)
{
    setObjectName(u"card"_s);
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(18, 14, 18, 14);
    col->setSpacing(10);
    auto* top = new QHBoxLayout;
    top->setSpacing(10);
    m_title->setStyleSheet(u"font-size: 13px; font-weight: 600;"_s);
    top->addWidget(m_title);
    top->addStretch();
    m_stats->setStyleSheet(u"font-size: 12px;"_s);
    top->addWidget(m_stats);
    m_action->setObjectName(u"download-"_s + model.id);
    top->addWidget(m_action);
    col->addLayout(top);
    m_bar->setTextVisible(false);
    m_bar->setRange(0, kBarSteps);
    m_bar->setAccessibleName(tr("Download progress"));
    col->addWidget(m_bar);
    QLabel* source =
        makeLabel(tr("From <span style=\"font-family: monospace\">%1</span> at version "
                     "<span style=\"font-family: monospace\">%2</span><br>Checked against its SHA-256 "
                     "fingerprint before first use · licence %3")
                      .arg(model.repository().toHtmlEscaped(), model.revision.left(7).toHtmlEscaped(),
                           model.licence.toHtmlEscaped()),
                  u"small"_s);
    source->setTextFormat(Qt::RichText);
    source->setWordWrap(true);
    col->addWidget(source);
    connect(m_action, &QPushButton::clicked, this, [this] { Q_EMIT actionClicked(m_modelId); });
    update(model, 0);
}

void DownloadPanel::update(const ModelInfo& model, double speed)
{
    const View v = viewOf(model, speed);
    m_title->setText(v.title);
    m_stats->setText(v.stats);
    m_action->setText(v.action);
    m_action->setVisible(!v.action.isEmpty());
    // Download is the step's main action until it starts.
    m_action->setProperty("kind",
                          model.downloadState.isEmpty() && !model.installed ? u"primary"_s : QString());
    repolish(m_action);
    const qint64 total = totalOf(model);
    m_bar->setValue(total > 0 ? static_cast<int>(model.received * kBarSteps / total) : 0);
    m_bar->setVisible(!model.installed);
}

} // namespace paraphrase::app
