#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>

#include <memory>
#include <optional>

namespace paraphrase::app::history {

class SecretSource;

/// One finished run (settings-privacy.html note 2). The text, result and options are sealed on
/// disk; the rest stays in clear so the list can show and filter it.
struct Entry
{
    qint64 id = 0;
    QDateTime when;
    QString skillId;
    /// The action as the list shows it: "Tone · Elaborate · into Hindi".
    QString label;
    /// The app the text came from; empty for the main window.
    QString app;
    QString text;
    QString result;
    /// The run's options as JSON, for Use again.
    QByteArray options;
};

struct Stats
{
    int count = 0;
    QDateTime oldest;
    qint64 bytes = 0;
};

/// The history file (ADR-017): `history.sqlite`, its fields sealed with a key derived from
/// the desktop's secret and the file's own salt.
class Store
{
public:
    enum class OpenError : quint8
    {
        NoSecret, ///< no keyring answered
        WrongKey, ///< the file was sealed with another secret
        Failed,   ///< the file could not be opened or made
    };

    /// Opens or creates the file. A file that the secret does not open stays untouched.
    [[nodiscard]] static std::unique_ptr<Store> open(const QString& path, SecretSource& secrets,
                                                     OpenError* error = nullptr);
    /// Delete all: removes the file and forgets the secret.
    static bool destroy(const QString& path, SecretSource& secrets);
    /// Deletes entries older than `before` without the key (only the clear times are read), so
    /// old entries go at start even before the keyring is asked; how many went.
    static int pruneFile(const QString& path, const QDateTime& before);
    /// The app's file: <data folder>/history.sqlite.
    [[nodiscard]] static QString defaultPath();

    ~Store();
    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;
    Store(Store&&) = delete;
    Store& operator=(Store&&) = delete;

    /// Keeps `entry` (its id and, when empty, its time are set here); the new id, or 0.
    qint64 add(Entry entry);
    /// Every entry, newest first, opened in memory.
    [[nodiscard]] QList<Entry> entries() const;
    /// The result the user saw last: another version, or the edited one they copied.
    bool updateResult(qint64 id, const QString& result);
    bool remove(qint64 id);
    /// Deletes entries older than `before`; how many went.
    int prune(const QDateTime& before);
    [[nodiscard]] Stats stats() const;
    [[nodiscard]] QString path() const;

private:
    struct Private;
    explicit Store(std::unique_ptr<Private> priv);
    std::unique_ptr<Private> m_d;
};

} // namespace paraphrase::app::history
