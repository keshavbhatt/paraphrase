#include "history/history_keeper.h"

#include "history/secret_source.h"

#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTimer>

#include <chrono>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

namespace paraphrase::app::history {
namespace {

const QString kOn = u"history/on"_s;
const QString kKeepDays = u"history/keepDays"_s;
const QString kExcluded = u"history/excludedApps"_s;
const QString kSecretKind = u"history/secretKind"_s;
constexpr int kDefaultKeepDays = 30;

} // namespace

Keeper::Keeper(QObject* parent)
    : Keeper(Store::defaultPath(), [](const QString& kind) { return desktopSecretSource(kind); }, parent)
{}

Keeper::Keeper(QString path, SecretFactory secrets, QObject* parent)
    : QObject(parent)
    , m_path(std::move(path))
    , m_secrets(std::move(secrets))
    , m_daily(new QTimer(this))
{
    m_daily->setInterval(24h);
    connect(m_daily, &QTimer::timeout, this, &Keeper::prune);
    m_daily->start();
    QTimer::singleShot(0, this, &Keeper::prune);
}

Keeper::~Keeper() = default;

bool Keeper::isOn()
{
    return QSettings().value(kOn, false).toBool();
}

Store* Keeper::store()
{
    if (m_store) {
        return m_store.get();
    }
    QSettings settings;
    const QString kind = settings.value(kSecretKind).toString();
    m_source = m_secrets(kind);
    if (!m_source) {
        m_problem = Problem::NoKeyring;
        return nullptr;
    }
    Store::OpenError error{};
    m_store = Store::open(m_path, *m_source, &error);
    if (!m_store) {
        switch (error) {
        case Store::OpenError::NoSecret:
            m_problem = Problem::NoKeyring;
            break;
        case Store::OpenError::WrongKey:
            m_problem = Problem::WrongKey;
            break;
        case Store::OpenError::Failed:
            m_problem = Problem::Failed;
            break;
        }
        return nullptr;
    }
    m_problem = Problem::None;
    // Later runs ask the same keyring, so the secret stays the same.
    settings.setValue(kSecretKind, m_source->kind());
    return m_store.get();
}

Keeper::Problem Keeper::turnOn()
{
    if (store() == nullptr) {
        return m_problem;
    }
    QSettings().setValue(kOn, true);
    Q_EMIT changed();
    return Problem::None;
}

void Keeper::turnOff(bool deleteEntries)
{
    QSettings().setValue(kOn, false);
    if (deleteEntries) {
        deleteAll();
    }
    Q_EMIT changed();
}

int Keeper::keepDays()
{
    return QSettings().value(kKeepDays, kDefaultKeepDays).toInt();
}

void Keeper::setKeepDays(int days)
{
    QSettings().setValue(kKeepDays, days);
    prune();
    Q_EMIT changed();
}

QStringList Keeper::excludedApps()
{
    static const QStringList kPasswordManagers{u"KeePassXC"_s, u"Bitwarden"_s, u"1Password"_s, u"KeePass"_s};
    return QSettings().value(kExcluded, kPasswordManagers).toStringList();
}

void Keeper::setExcludedApps(const QStringList& apps)
{
    QSettings().setValue(kExcluded, apps);
    Q_EMIT changed();
}

bool Keeper::isExcluded(const QString& app)
{
    return !app.isEmpty() && excludedApps().contains(app, Qt::CaseInsensitive);
}

qint64 Keeper::record(const Entry& entry)
{
    if (!isOn() || isExcluded(entry.app) || entry.result.isEmpty()) {
        return 0;
    }
    Store* s = store();
    const qint64 id = s == nullptr ? 0 : s->add(entry);
    if (id != 0) {
        Q_EMIT changed();
    }
    return id;
}

bool Keeper::update(qint64 id, const QString& result)
{
    if (id == 0 || !isOn() || result.isEmpty()) {
        return false;
    }
    Store* s = store();
    const bool ok = s != nullptr && s->updateResult(id, result);
    if (ok) {
        Q_EMIT changed();
    }
    return ok;
}

QList<Entry> Keeper::entries()
{
    const Store* s = isOn() || QFile::exists(m_path) ? store() : nullptr;
    return s == nullptr ? QList<Entry>() : s->entries();
}

bool Keeper::remove(qint64 id)
{
    Store* s = store();
    const bool ok = s != nullptr && s->remove(id);
    if (ok) {
        Q_EMIT changed();
    }
    return ok;
}

Stats Keeper::stats()
{
    // Counting needs no key: only clear columns are read.
    if (m_store) {
        return m_store->stats();
    }
    Stats s;
    s.bytes = QFileInfo(m_path).size();
    if (const Store* opened = s.bytes > 0 ? store() : nullptr) {
        return opened->stats();
    }
    return s;
}

bool Keeper::deleteAll()
{
    m_store.reset();
    if (!m_source) {
        m_source = m_secrets(QSettings().value(kSecretKind).toString());
    }
    bool ok = true;
    if (m_source) {
        ok = Store::destroy(m_path, *m_source);
    } else {
        ok = !QFile::exists(m_path) || QFile::remove(m_path);
    }
    m_source.reset();
    QSettings().remove(kSecretKind);
    Q_EMIT changed();
    return ok;
}

int Keeper::prune()
{
    const int days = keepDays();
    if (days <= 0) {
        return 0;
    }
    const QDateTime before = QDateTime::currentDateTime().addDays(-days);
    const int gone = m_store ? m_store->prune(before) : Store::pruneFile(m_path, before);
    if (gone > 0) {
        Q_EMIT changed();
    }
    return gone;
}

} // namespace paraphrase::app::history
