#include "daemon_service.h"

#include "models/tier_selector.h"
#include "skills/request_inputs.h"

#include <QCoreApplication>
#include <QDBusMessage>
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QUuid>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcApi, "paraphrase.api")

namespace paraphrase::api {
namespace {

const QString kErrorPrefix = u"com.ktechpit.paraphrase.Error."_s;
constexpr qint64 kProgressIntervalMs = 250;

QString stateName(models::DownloadStatus::State state)
{
    using State = models::DownloadStatus::State;
    switch (state) {
    case State::Waiting:
        return u"waiting"_s;
    case State::Downloading:
        return u"downloading"_s;
    case State::Verifying:
        return u"verifying"_s;
    case State::Paused:
        return u"paused"_s;
    case State::Failed:
        return u"failed"_s;
    case State::Installed:
        return u"installed"_s;
    }
    return {};
}

/// Resident memory of this process in MB (VmRSS), for Status.
double residentMb()
{
    QFile f(u"/proc/self/status"_s);
    if (!f.open(QIODevice::ReadOnly)) {
        return 0.0;
    }
    for (const QByteArray& line : f.readAll().split('\n')) {
        if (line.startsWith("VmRSS:")) {
            return line.mid(6).trimmed().split(' ').front().toDouble() / 1024.0;
        }
    }
    return 0.0;
}

} // namespace

DaemonService::DaemonService(ServiceSetup setup, QObject* parent)
    : QObject(parent)
    , m_setup(std::move(setup))
    , m_detector(util::LanguageData::builtIn())
    , m_store(m_setup.modelsRoot)
    , m_worker(new InferenceWorker(m_setup.engineFactory, m_detector))
    , m_downloader(m_store, nullptr, m_setup.urlResolver)
{
    connect(&m_downloader, &models::ModelDownloader::changed, this, &DaemonService::onDownloadChanged);
    for (const auto& s : m_setup.skills.skills()) {
        m_maxVariants = std::max(m_maxVariants, s.variants);
    }
    m_thread.setObjectName(u"inference"_s);
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &InferenceWorker::token, this, &DaemonService::onToken);
    connect(m_worker, &InferenceWorker::retrying, this, &DaemonService::onRetrying);
    connect(m_worker, &InferenceWorker::finished, this, &DaemonService::onFinished);
    connect(m_worker, &InferenceWorker::failed, this, &DaemonService::onFailed);
    connect(m_worker, &InferenceWorker::idle, this, &DaemonService::onIdle);
    connect(m_worker, &InferenceWorker::modelChanged, this,
            [this](const QString& id) { m_loadedModel = id; });
    m_thread.start();

    m_unloadTimer.setSingleShot(true);
    m_unloadTimer.setInterval(m_setup.idleUnloadMs);
    connect(&m_unloadTimer, &QTimer::timeout, this,
            [this] { QMetaObject::invokeMethod(m_worker, &InferenceWorker::unload, Qt::QueuedConnection); });
    m_exitTimer.setSingleShot(true);
    m_exitTimer.setInterval(m_setup.idleExitMs);
    connect(&m_exitTimer, &QTimer::timeout, this, &DaemonService::idleTimeout);
    restartIdleTimers();
}

DaemonService::~DaemonService()
{
    if (!m_running.isEmpty()) {
        m_worker->cancel(m_running);
    }
    m_thread.quit();
    m_thread.wait();
}

bool DaemonService::exportOn(QDBusConnection connection)
{
    return connection.registerObject(QString::fromLatin1(kObjectPath), this,
                                     QDBusConnection::ExportScriptableSlots |
                                         QDBusConnection::ExportScriptableSignals);
}

DaemonService::Client DaemonService::caller() const
{
    if (!calledFromDBus()) {
        return {};
    }
    return {.connection = connection().name(), .service = message().service()};
}

void DaemonService::replyError(const QString& name, const QString& message)
{
    if (calledFromDBus()) {
        sendErrorReply(kErrorPrefix + name, message);
    }
}

void DaemonService::sendTo(const QString& requestId, const char* signal, const QVariantList& args)
{
    const auto it = m_clients.constFind(requestId);
    if (it == m_clients.constEnd() || it->connection.isEmpty()) {
        return;
    }
    const QString path = QString::fromLatin1(kObjectPath);
    const QString iface = QString::fromLatin1(kInterface);
    const QString name = QString::fromLatin1(signal);
    // A peer connection has one client; on the bus, address the one that asked.
    QDBusMessage msg = it->service.isEmpty()
                           ? QDBusMessage::createSignal(path, iface, name)
                           : QDBusMessage::createTargetedSignal(it->service, path, iface, name);
    msg.setArguments(args);
    QDBusConnection(it->connection).send(msg);
}

QSet<QString> DaemonService::installedModels() const
{
    QSet<QString> out;
    for (const auto& m : m_setup.catalog.models) {
        if (m.role == u"model"_s && m_store.isInstalled(m)) {
            out.insert(m.id);
        }
    }
    return out;
}

QString DaemonService::buildJob(const QString& skillId, const QVariantMap& input, Job& job) const
{
    const skills::Skill* skill = m_setup.skills.find(skillId);
    if (skill == nullptr) {
        return tr("Unknown skill \"%1\".").arg(skillId);
    }
    static const QSet<QString> kKeys = {u"text"_s, u"context"_s, u"target_lang"_s};
    for (auto it = input.begin(); it != input.end(); ++it) {
        if (!kKeys.contains(it.key()) || it.value().typeId() != QMetaType::QString) {
            return tr("Unknown input \"%1\": text, context and target_lang are strings.").arg(it.key());
        }
    }
    const skills::RequestFields fields{
        .text = input.value(u"text"_s).toString(),
        .context = input.value(u"context"_s).toString(),
        .targetLang = input.value(u"target_lang"_s).toString(),
    };
    if (skill->baseId == u"reply"_s ? fields.context.trimmed().isEmpty() : fields.text.trimmed().isEmpty()) {
        return skill->baseId == u"reply"_s ? tr("Reply needs the message to reply to (context).")
                                           : tr("%1 needs some text.").arg(skill->title);
    }
    // Only a skill whose prompt names the target language can write in another one.
    if (!fields.targetLang.isEmpty() && !skill->templ.contains(u"target_lang"_s)) {
        return tr("%1 keeps the language of the text; it cannot write in another language.")
            .arg(skill->title);
    }
    if (!fields.targetLang.isEmpty() && m_detector.plainName(fields.targetLang).isEmpty()) {
        return tr("Unknown language \"%1\".").arg(fields.targetLang);
    }
    job.skill = *skill;
    job.inputs = skills::buildInputs(skill->baseId, fields, m_detector);
    return {};
}

bool DaemonService::chooseModel(Job& job, const models::ModelEntry* requested) const
{
    const models::TierSelector selector(m_setup.catalog, m_setup.hardware);
    const models::ModelEntry* model =
        requested != nullptr ? requested : selector.modelFor(job.skill.tier, installedModels());
    if (model == nullptr) {
        return false;
    }
    const models::ModelEntry* draft = m_setup.catalog.draftFor(model->id);
    job.modelId = model->id;
    job.model = engine::EngineConfig{
        .modelPath = m_store.modelPath(*model),
        .draftModelPath =
            draft != nullptr && m_store.isInstalled(*draft) ? m_store.modelPath(*draft) : QString(),
        .threads = m_setup.hardware.threads,
        .gpuLayers = 0, // CPU only (ADR-016)
        .contextSize = model->ctx,
        .maxVariants = m_maxVariants,
        .flashAttention = true,
    };
    return true;
}

QString DaemonService::RunSkill(const QString& skillId, const QVariantMap& input, const QVariantMap& options)
{
    Job job;
    if (const QString problem = buildJob(skillId, input, job); !problem.isEmpty()) {
        replyError(u"InvalidInput"_s, problem);
        return {};
    }
    // options.model: a model chosen by the user or the eval, instead of the tier's.
    const models::ModelEntry* requested = nullptr;
    if (const QString modelId = options.value(u"model"_s).toString(); !modelId.isEmpty()) {
        requested = m_setup.catalog.find(modelId);
        if (requested == nullptr || requested->role != u"model"_s) {
            replyError(u"InvalidInput"_s, tr("Unknown model \"%1\".").arg(modelId));
            return {};
        }
        if (!m_store.isInstalled(*requested)) {
            replyError(u"NoModel"_s, tr("%1 is not installed.").arg(requested->name));
            return {};
        }
    }
    for (auto it = options.begin(); it != options.end(); ++it) {
        if (it.key() != u"model"_s) {
            replyError(u"InvalidInput"_s, tr("Unknown option \"%1\".").arg(it.key()));
            return {};
        }
    }
    if (!chooseModel(job, requested)) {
        replyError(u"NoModel"_s, tr("No model is installed. Download one in Settings, or run "
                                    "\"paraphrase models download fast\"."));
        return {};
    }
    job.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const Client client = caller();
    cancelClientRequests(client);
    m_queue.removeIf([](const Job& j) { return j.warmOnly; }); // a real request beats read-ahead
    m_clients.insert(job.id, client);
    qCInfo(lcApi) << "request" << job.id << job.skill.id << "on" << job.modelId;
    m_queue.append(job);
    dispatchNext();
    return job.id;
}

void DaemonService::Warm(const QString& skillId, const QVariantMap& input)
{
    Job job;
    if (!buildJob(skillId, input, job).isEmpty()) {
        return;
    }
    if (std::ranges::any_of(m_queue, [](const Job& j) { return !j.warmOnly; })) {
        return; // real work is waiting
    }
    if (!chooseModel(job)) {
        return;
    }
    job.id = u"warm-"_s + QUuid::createUuid().toString(QUuid::WithoutBraces);
    job.warmOnly = true;
    m_queue.removeIf([](const Job& j) { return j.warmOnly; });
    m_queue.append(job);
    dispatchNext();
}

void DaemonService::cancelClientRequests(const Client& client)
{
    if (client.connection.isEmpty()) {
        return;
    }
    QStringList mine;
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        if (it.value() == client) {
            mine.append(it.key());
        }
    }
    for (const QString& id : mine) {
        Cancel(id);
    }
}

void DaemonService::Cancel(const QString& requestId)
{
    const auto it = m_clients.constFind(requestId);
    if (it == m_clients.constEnd() || (calledFromDBus() && !(it.value() == caller()))) {
        return; // unknown, finished, or someone else's
    }
    if (requestId == m_running) {
        m_worker->cancel(requestId); // ends with failed(id, "cancelled")
        return;
    }
    m_queue.removeIf([&](const Job& j) { return j.id == requestId; });
    sendTo(requestId, "Error", {requestId, u"cancelled"_s, tr("Cancelled.")});
    finishRequest(requestId);
}

void DaemonService::dispatchNext()
{
    if (m_busy || m_queue.isEmpty()) {
        return;
    }
    m_unloadTimer.stop();
    m_exitTimer.stop();
    const Job job = m_queue.takeFirst();
    m_busy = true;
    m_running = job.warmOnly ? QString() : job.id;
    QMetaObject::invokeMethod(m_worker, [worker = m_worker, job] { worker->run(job); }, Qt::QueuedConnection);
}

void DaemonService::finishRequest(const QString& id)
{
    m_clients.remove(id);
}

void DaemonService::onIdle()
{
    m_busy = false;
    m_running.clear();
    dispatchNext();
    if (!m_busy) {
        restartIdleTimers();
    }
}

void DaemonService::restartIdleTimers()
{
    m_unloadTimer.start();
    if (!downloading()) { // a download keeps the daemon up
        m_exitTimer.start();
    }
}

bool DaemonService::downloading() const
{
    return std::ranges::any_of(m_downloader.downloads(), [](const models::DownloadStatus& s) {
        return s.state == models::DownloadStatus::State::Waiting ||
               s.state == models::DownloadStatus::State::Downloading ||
               s.state == models::DownloadStatus::State::Verifying;
    });
}

void DaemonService::onDownloadChanged(const models::DownloadStatus& status)
{
    using State = models::DownloadStatus::State;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    // Bytes arrive many times a second; state changes always go out.
    if (status.state == State::Downloading &&
        now - m_lastProgressMs.value(status.modelId) < kProgressIntervalMs) {
        return;
    }
    m_lastProgressMs.insert(status.modelId, now);
    if (downloading()) {
        m_exitTimer.stop();
    } else if (!m_busy) {
        restartIdleTimers();
    }
    const QJsonObject o{
        {u"model"_s, status.modelId}, {u"state"_s, stateName(status.state)}, {u"received"_s, status.received},
        {u"total"_s, status.total},   {u"verified"_s, status.verified},      {u"error"_s, status.error},
    };
    Q_EMIT DownloadProgress(QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
}

QJsonObject DaemonService::modelJson(const models::ModelEntry& m) const
{
    const models::TierSelector selector(m_setup.catalog, m_setup.hardware);
    const auto& file = m.files.front();
    QJsonObject o{
        {u"id"_s, m.id},
        {u"name"_s, m.name},
        {u"role"_s, m.role},
        {u"tiers"_s, QJsonArray::fromStringList(m.tiers)},
        {u"draft_for"_s, QJsonArray::fromStringList(m.draftFor)},
        {u"installed"_s, m_store.isInstalled(m)},
        {u"fits"_s, m.role != u"model"_s || selector.fits(m)},
        {u"recommended"_s, m.tiers.contains(u"fast"_s)},
        {u"size"_s, m.downloadSize()},
        {u"on_disk"_s, m_store.usedBytes(m)},
        {u"ram_required_gb"_s, m.ramRequiredGb},
        {u"memory_gb"_s, m.memoryGb},
        {u"licence"_s, m.licence},
        {u"licence_url"_s, m.licenceUrl},
        // PLAN 8: the exact source, so a careful user can check the download.
        {u"source"_s, m.url(file).toString()},
        {u"revision"_s, m.revision},
        {u"sha256"_s, file.sha256},
        {u"path"_s, m_store.modelPath(m)},
    };
    for (const auto& d : m_downloader.downloads()) {
        if (d.modelId == m.id) {
            o.insert(u"download"_s, QJsonObject{
                                        {u"state"_s, stateName(d.state)},
                                        {u"received"_s, d.received},
                                        {u"total"_s, d.total},
                                        {u"error"_s, d.error},
                                    });
        }
    }
    return o;
}

QString DaemonService::ListModels() const
{
    QJsonArray out;
    for (const auto& m : m_setup.catalog.models) {
        if (m.isOffered()) {
            out.append(modelJson(m));
        }
    }
    return QString::fromUtf8(QJsonDocument(out).toJson(QJsonDocument::Compact));
}

void DaemonService::DownloadModel(const QString& modelId)
{
    const models::ModelEntry* m = m_setup.catalog.find(modelId);
    if (m == nullptr || m->role != u"model"_s) {
        replyError(u"InvalidInput"_s, tr("Unknown model \"%1\".").arg(modelId));
        return;
    }
    qCInfo(lcApi) << "download" << modelId;
    m_exitTimer.stop();
    m_downloader.start(*m);
    if (const models::ModelEntry* draft = m_setup.catalog.draftFor(modelId)) {
        m_downloader.start(*draft); // the speed-up comes with it (ADR-004)
    }
}

void DaemonService::PauseDownload(const QString& modelId)
{
    m_downloader.pause(modelId);
    if (const models::ModelEntry* draft = m_setup.catalog.draftFor(modelId)) {
        m_downloader.pause(draft->id);
    }
}

void DaemonService::DeleteModel(const QString& modelId)
{
    const models::ModelEntry* m = m_setup.catalog.find(modelId);
    if (m == nullptr || m->role != u"model"_s) {
        replyError(u"InvalidInput"_s, tr("Unknown model \"%1\".").arg(modelId));
        return;
    }
    const models::ModelEntry* draft = m_setup.catalog.draftFor(modelId);
    const bool active = std::ranges::any_of(m_downloader.downloads(), [&](const models::DownloadStatus& s) {
        return s.modelId == modelId || (draft != nullptr && s.modelId == draft->id);
    });
    if (modelId == m_loadedModel || active ||
        std::ranges::any_of(m_queue, [&](const Job& j) { return j.modelId == modelId; })) {
        replyError(u"Busy"_s, tr("%1 is in use or downloading; try again when it is idle.").arg(m->name));
        return;
    }
    qCInfo(lcApi) << "delete" << modelId;
    for (const models::ModelEntry* e : {m, draft}) {
        if (e == nullptr) {
            continue;
        }
        if (const auto removed = m_store.remove(*e); !removed.ok()) {
            replyError(u"Failed"_s, removed.error());
            return;
        }
    }
}

void DaemonService::onToken(const QString& id, int variant, const QString& text)
{
    sendTo(id, "Token", {id, variant, text});
}

void DaemonService::onRetrying(const QString& id, const QString& reason)
{
    sendTo(id, "Retrying", {id, reason});
}

void DaemonService::onFinished(const QString& id, const QString& result)
{
    sendTo(id, "Done", {id, result});
    finishRequest(id);
}

void DaemonService::onFailed(const QString& id, const QString& code, const QString& message)
{
    qCInfo(lcApi) << "request" << id << "failed:" << code;
    sendTo(id, "Error", {id, code, message});
    finishRequest(id);
}

QString DaemonService::ListSkills() const
{
    QJsonArray out;
    for (const auto& s : m_setup.skills.skills()) {
        out.append(QJsonObject{
            {u"id"_s, s.id},
            {u"base"_s, s.baseId},
            {u"title"_s, s.title},
            {u"tier"_s, s.tier},
            {u"output"_s, s.outputType()},
            {u"variants"_s, s.variants},
            {u"max_input_tokens"_s, s.maxInputTokens},
        });
    }
    return QString::fromUtf8(QJsonDocument(out).toJson(QJsonDocument::Compact));
}

QString DaemonService::Status()
{
    const auto& hw = m_setup.hardware;
    const QJsonObject o{
        {u"version"_s, QCoreApplication::applicationVersion()},
        {u"model"_s, m_loadedModel},
        {u"backend"_s, u"cpu"_s},
        {u"threads"_s, hw.threads},
        {u"busy"_s, m_busy},
        {u"queue"_s, m_queue.size()},
        {u"memory_mb"_s, residentMb()},
        {
            u"computer"_s,
            QJsonObject{
                {u"cpu"_s, hw.cpuModel},
                {u"memory_gb"_s, hw.totalMemoryGb()},
                {u"cpus"_s, hw.logicalCpus},
                {u"features"_s, QJsonArray::fromStringList(hw.cpuFeatures)},
            },
        },
        {u"models_dir"_s, m_store.root()},
        {u"installed"_s, QJsonArray::fromStringList(installedModels().values())},
    };
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

} // namespace paraphrase::api
