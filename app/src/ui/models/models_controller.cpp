#include "ui/models/models_controller.h"

#include "daemon_client.h"

#include <QDBusMessage>
#include <QDateTime>
#include <QJsonDocument>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr double kSpeedSmoothing = 0.3;

QString text(const QDBusMessage& reply)
{
    return reply.arguments().value(0).toString();
}

} // namespace

ModelsController::ModelsController(client::DaemonClient& client, QObject* parent)
    : QObject(parent)
    , m_client(client)
{
    connect(&m_client, &client::DaemonClient::downloadProgress, this, &ModelsController::onProgress);
}

ModelsController::~ModelsController() = default;

void ModelsController::refresh()
{
    m_models = parseModels(text(m_client.call(u"ListModels"_s, {})));
    m_status = QJsonDocument::fromJson(text(m_client.call(u"Status"_s, {})).toUtf8()).object();
    Q_EMIT changed();
}

const ModelInfo* ModelsController::model(const QString& id) const
{
    for (const ModelInfo& m : m_models) {
        if (m.id == id) {
            return &m;
        }
    }
    return nullptr;
}

const ModelInfo* ModelsController::offerFor(const QString& tier) const
{
    const ModelInfo* pick = nullptr;
    for (const ModelInfo& m : m_models) {
        if (m.isModel() && m.tiers.contains(tier) && (pick == nullptr || m.recommended)) {
            pick = &m;
        }
    }
    return pick;
}

QJsonObject ModelsController::computer() const
{
    return m_status.value(u"computer"_s).toObject();
}

QString ModelsController::tierModel(const QString& tier) const
{
    return m_status.value(u"tier_models"_s).toObject().value(tier).toString();
}

bool ModelsController::speedUp() const
{
    return m_status.value(u"speed_up"_s).toBool(true);
}

QString ModelsController::modelsDir() const
{
    return m_status.value(u"models_dir"_s).toString();
}

QString ModelsController::callFor(const QString& method, const QVariantList& args)
{
    const QDBusMessage reply = m_client.call(method, args);
    return reply.type() == QDBusMessage::ErrorMessage ? reply.errorMessage() : QString();
}

QString ModelsController::download(const QString& id)
{
    // Only ever on a click: the user chose this download.
    QString problem = callFor(u"DownloadModel"_s, {id});
    refresh();
    return problem;
}

QString ModelsController::pause(const QString& id)
{
    QString problem = callFor(u"PauseDownload"_s, {id});
    refresh();
    return problem;
}

QString ModelsController::remove(const QString& id)
{
    // Only after the user confirmed it in the window.
    QString problem = callFor(u"DeleteModel"_s, {id});
    refresh();
    return problem;
}

QString ModelsController::setTierModel(const QString& tier, const QString& id)
{
    QString problem = callFor(u"SetTierModel"_s, {tier, id});
    refresh();
    return problem;
}

QString ModelsController::setSpeedUp(bool on)
{
    QString problem = callFor(u"SetSpeedUp"_s, {on});
    refresh();
    return problem;
}

void ModelsController::onProgress(const QString& status)
{
    const QJsonObject s = QJsonDocument::fromJson(status.toUtf8()).object();
    const QString id = s.value(u"model"_s).toString();
    for (ModelInfo& m : m_models) {
        if (m.id != id) {
            continue;
        }
        const QString before = m.downloadState;
        m.applyProgress(s);
        trackSpeed(m);
        if (before != m.downloadState) {
            Q_EMIT stateChanged(id);
        } else {
            Q_EMIT progress(id);
        }
        return;
    }
}

void ModelsController::trackSpeed(const ModelInfo& m)
{
    // A smoothed speed for "8.4 MB/s · about 4 min left".
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 lastMs = m_lastMs.value(m.id, -1);
    const qint64 lastBytes = m_lastBytes.value(m.id);
    if (lastMs >= 0 && now > lastMs && m.received >= lastBytes) {
        const double instant =
            static_cast<double>(m.received - lastBytes) * 1000.0 / static_cast<double>(now - lastMs);
        const double old = m_speed.value(m.id, instant);
        m_speed.insert(m.id, (old * (1.0 - kSpeedSmoothing)) + (instant * kSpeedSmoothing));
    }
    m_lastMs.insert(m.id, now);
    m_lastBytes.insert(m.id, m.received);
}

} // namespace paraphrase::app
