#pragma once

#include "model_info.h"

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>

namespace paraphrase::client {
class DaemonClient;
}

namespace paraphrase::app {

/// The model calls of the daemon's API (DOCS/api.md) for the setup and settings windows:
/// what is offered and installed, downloads and their progress, removal, and the user's
/// choices of a model per tier and the speed-up. Widgets read it; they never call D-Bus.
class ModelsController : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ModelsController)

public:
    explicit ModelsController(client::DaemonClient& client, QObject* parent = nullptr);
    ~ModelsController() override;

    /// Reads ListModels and Status again; emits changed(). Pointers from model() and offerFor()
    /// do not survive it, nor the actions below, which read the lists again.
    void refresh();

    [[nodiscard]] const QList<ModelInfo>& models() const { return m_models; }
    [[nodiscard]] const ModelInfo* model(const QString& id) const;
    /// The catalog's model for a tier: the recommended one, else the first.
    [[nodiscard]] const ModelInfo* offerFor(const QString& tier) const;
    /// Status().computer: memory_gb, cpus, cpu.
    [[nodiscard]] QJsonObject computer() const;
    /// The model each tier runs on now (Status().tier_models).
    [[nodiscard]] QString tierModel(const QString& tier) const;
    [[nodiscard]] bool speedUp() const;
    [[nodiscard]] QString modelsDir() const;
    /// Bytes per second of a running download, smoothed; 0 when unknown.
    [[nodiscard]] double speed(const QString& id) const { return m_speed.value(id); }

    // Each returns "" or the daemon's message for the user.
    QString download(const QString& id);
    QString pause(const QString& id);
    QString remove(const QString& id);
    QString setTierModel(const QString& tier, const QString& id);
    QString setSpeedUp(bool on);

Q_SIGNALS:
    void changed();                       ///< the lists and the status were read again
    void progress(const QString& id);     ///< bytes of a download moved on
    void stateChanged(const QString& id); ///< a download started, paused, failed or finished

private:
    void onProgress(const QString& status);
    void trackSpeed(const ModelInfo& m);
    [[nodiscard]] QString callFor(const QString& method, const QVariantList& args);

    client::DaemonClient& m_client;
    QList<ModelInfo> m_models;
    QJsonObject m_status;
    QHash<QString, qint64> m_lastBytes;
    QHash<QString, qint64> m_lastMs;
    QHash<QString, double> m_speed;
};

} // namespace paraphrase::app
