#include "history/history_store.h"

#include "history/history_crypto.h"
#include "history/secret_source.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

#include <atomic>

using namespace Qt::StringLiterals;

namespace paraphrase::app::history {
namespace {

constexpr int kSaltBytes = 32;
/// Sealed into the file once: a key that opens it is the file's key.
constexpr QByteArrayView kCheck("paraphrase history");

QString connectionName()
{
    static std::atomic<int> counter{0};
    return u"paraphrase-history-%1"_s.arg(++counter);
}

bool exec(const QSqlDatabase& db, const QString& sql)
{
    QSqlQuery q(db);
    return q.exec(sql);
}

std::optional<QByteArray> meta(const QSqlDatabase& db, const QString& key)
{
    QSqlQuery q(db);
    q.prepare(u"SELECT value FROM meta WHERE key = ?"_s);
    q.addBindValue(key);
    if (!q.exec() || !q.next()) {
        return std::nullopt;
    }
    return q.value(0).toByteArray();
}

bool setMeta(const QSqlDatabase& db, const QString& key, const QByteArray& value)
{
    QSqlQuery q(db);
    q.prepare(u"INSERT OR REPLACE INTO meta (key, value) VALUES (?, ?)"_s);
    q.addBindValue(key);
    q.addBindValue(value);
    return q.exec();
}

} // namespace

struct Store::Private
{
    QString path;
    QString connection;
    QByteArray key;

    [[nodiscard]] QSqlDatabase db() const { return QSqlDatabase::database(connection, false); }
    [[nodiscard]] QString openText(const QVariant& sealed) const
    {
        return QString::fromUtf8(history::open(key, sealed.toByteArray()).value_or(QByteArray()));
    }
};

QString Store::defaultPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + u"/history.sqlite"_s;
}

std::unique_ptr<Store> Store::open(const QString& path, SecretSource& secrets, OpenError* error)
{
    const auto fail = [error](OpenError e) {
        if (error != nullptr) {
            *error = e;
        }
        return nullptr;
    };
    const std::optional<QByteArray> secret = secrets.fetch();
    if (!secret || secret->isEmpty()) {
        return fail(OpenError::NoSecret);
    }
    QDir().mkpath(QFileInfo(path).absolutePath());

    auto priv = std::make_unique<Private>();
    priv->path = path;
    priv->connection = connectionName();
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, priv->connection);
        db.setDatabaseName(path);
        bool ok = db.open();
        // Deleted rows are overwritten, not left in free pages.
        ok = ok && exec(db, u"PRAGMA secure_delete = ON"_s);
        ok = ok && exec(db, u"CREATE TABLE IF NOT EXISTS meta (key TEXT PRIMARY KEY, value BLOB)"_s);
        ok = ok &&
             exec(db, u"CREATE TABLE IF NOT EXISTS entries (id INTEGER PRIMARY KEY, at INTEGER NOT NULL, "
                      "skill TEXT NOT NULL, label TEXT NOT NULL, app TEXT NOT NULL, text BLOB, "
                      "result BLOB, options BLOB)"_s);
        ok = ok && exec(db, u"CREATE INDEX IF NOT EXISTS entries_at ON entries (at)"_s);
        std::optional<QByteArray> salt = ok ? meta(db, u"salt"_s) : std::nullopt;
        if (ok && !salt) {
            salt = randomBytes(kSaltBytes);
            ok = !salt->isEmpty() && setMeta(db, u"salt"_s, *salt);
        }
        if (ok) {
            priv->key = deriveKey(*secret + *salt);
            ok = !priv->key.isEmpty();
        }
        if (ok) {
            const std::optional<QByteArray> check = meta(db, u"check"_s);
            if (check && history::open(priv->key, *check) != kCheck.toByteArray()) {
                db.close();
                db = QSqlDatabase();
                QSqlDatabase::removeDatabase(priv->connection);
                return fail(OpenError::WrongKey);
            }
            ok = check.has_value() || setMeta(db, u"check"_s, seal(priv->key, kCheck.toByteArray()));
        }
        if (!ok) {
            db.close();
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(priv->connection);
            return fail(OpenError::Failed);
        }
    }
    return std::unique_ptr<Store>(new Store(std::move(priv)));
}

bool Store::destroy(const QString& path, SecretSource& secrets)
{
    secrets.forget();
    // The salt goes with the file, so even a secret that stays cannot open a copy's twin.
    const bool ok = !QFile::exists(path) || QFile::remove(path);
    for (const QString& extra : {u"-journal"_s, u"-wal"_s, u"-shm"_s}) {
        QFile::remove(path + extra);
    }
    return ok;
}

int Store::pruneFile(const QString& path, const QDateTime& before)
{
    if (!QFile::exists(path)) {
        return 0;
    }
    const QString name = connectionName();
    int gone = 0;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, name);
        db.setDatabaseName(path);
        if (db.open() && exec(db, u"PRAGMA secure_delete = ON"_s)) {
            QSqlQuery q(db);
            q.prepare(u"DELETE FROM entries WHERE at < ?"_s);
            q.addBindValue(before.toMSecsSinceEpoch());
            gone = q.exec() ? q.numRowsAffected() : 0;
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(name);
    return gone;
}

Store::Store(std::unique_ptr<Private> priv)
    : m_d(std::move(priv))
{}

Store::~Store()
{
    {
        QSqlDatabase db = m_d->db();
        db.close();
    }
    QSqlDatabase::removeDatabase(m_d->connection);
}

QString Store::path() const
{
    return m_d->path;
}

qint64 Store::add(Entry entry)
{
    if (!entry.when.isValid()) {
        entry.when = QDateTime::currentDateTime();
    }
    const QSqlDatabase db = m_d->db();
    QSqlQuery q(db);
    q.prepare(
        u"INSERT INTO entries (at, skill, label, app, text, result, options) VALUES (?, ?, ?, ?, ?, ?, ?)"_s);
    q.addBindValue(entry.when.toMSecsSinceEpoch());
    // A null QString binds as NULL; these columns hold "" for none.
    const auto text = [](const QString& s) { return s.isNull() ? u""_s : s; };
    q.addBindValue(text(entry.skillId));
    q.addBindValue(text(entry.label));
    q.addBindValue(text(entry.app));
    q.addBindValue(seal(m_d->key, entry.text.toUtf8()));
    q.addBindValue(seal(m_d->key, entry.result.toUtf8()));
    q.addBindValue(seal(m_d->key, entry.options));
    if (!q.exec()) {
        return 0;
    }
    return q.lastInsertId().toLongLong();
}

QList<Entry> Store::entries() const
{
    QList<Entry> out;
    const QSqlDatabase db = m_d->db();
    QSqlQuery q(db);
    if (!q.exec(
            u"SELECT id, at, skill, label, app, text, result, options FROM entries ORDER BY at DESC, id DESC"_s)) {
        return out;
    }
    while (q.next()) {
        Entry e;
        e.id = q.value(0).toLongLong();
        e.when = QDateTime::fromMSecsSinceEpoch(q.value(1).toLongLong());
        e.skillId = q.value(2).toString();
        e.label = q.value(3).toString();
        e.app = q.value(4).toString();
        e.text = m_d->openText(q.value(5));
        e.result = m_d->openText(q.value(6));
        e.options = history::open(m_d->key, q.value(7).toByteArray()).value_or(QByteArray());
        out.append(e);
    }
    return out;
}

bool Store::updateResult(qint64 id, const QString& result)
{
    const QSqlDatabase db = m_d->db();
    QSqlQuery q(db);
    q.prepare(u"UPDATE entries SET result = ? WHERE id = ?"_s);
    q.addBindValue(seal(m_d->key, result.toUtf8()));
    q.addBindValue(id);
    return q.exec() && q.numRowsAffected() == 1;
}

bool Store::remove(qint64 id)
{
    const QSqlDatabase db = m_d->db();
    QSqlQuery q(db);
    q.prepare(u"DELETE FROM entries WHERE id = ?"_s);
    q.addBindValue(id);
    return q.exec() && q.numRowsAffected() == 1;
}

int Store::prune(const QDateTime& before)
{
    const QSqlDatabase db = m_d->db();
    QSqlQuery q(db);
    q.prepare(u"DELETE FROM entries WHERE at < ?"_s);
    q.addBindValue(before.toMSecsSinceEpoch());
    return q.exec() ? q.numRowsAffected() : 0;
}

Stats Store::stats() const
{
    Stats s;
    const QSqlDatabase db = m_d->db();
    QSqlQuery q(db);
    if (q.exec(u"SELECT COUNT(*), MIN(at) FROM entries"_s) && q.next()) {
        s.count = q.value(0).toInt();
        if (s.count > 0) {
            s.oldest = QDateTime::fromMSecsSinceEpoch(q.value(1).toLongLong());
        }
    }
    s.bytes = QFileInfo(m_d->path).size();
    return s;
}

} // namespace paraphrase::app::history
