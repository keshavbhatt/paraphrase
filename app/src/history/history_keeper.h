#pragma once

#include "history/history_store.h"

#include <QObject>
#include <QStringList>

#include <functional>
#include <memory>

class QTimer;

namespace paraphrase::app::history {

class SecretSource;

/// The history as the app uses it (settings-privacy.html, main-window-history.html): whether
/// it is on, how long entries stay, the apps it never keeps text from, and the store, opened
/// on first use so the keyring is asked only when the history is needed.
class Keeper : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Keeper)

public:
    enum class Problem : quint8
    {
        None,
        NoKeyring, ///< no Secret portal or Secret Service answered
        WrongKey,  ///< the file is sealed with a secret the keyring no longer has
        Failed,    ///< the file could not be opened or made
    };
    using SecretFactory = std::function<std::unique_ptr<SecretSource>(const QString& kind)>;

    /// The app's history: its data folder, the desktop's keyring.
    explicit Keeper(QObject* parent = nullptr);
    /// For tests: another file and keyring.
    Keeper(QString path, SecretFactory secrets, QObject* parent = nullptr);
    ~Keeper() override;

    [[nodiscard]] static bool isOn();
    /// Opens the store (the keyring may ask) and keeps history from now on.
    Problem turnOn();
    /// Stops keeping history; `deleteEntries` also deletes what was kept (Delete all).
    void turnOff(bool deleteEntries);
    /// Why the last open failed.
    [[nodiscard]] Problem problem() const { return m_problem; }

    /// 7, 30 (the default) or 365 days; 0 keeps entries until the user deletes them.
    [[nodiscard]] static int keepDays();
    void setKeepDays(int days);
    /// Apps whose text is never kept; password managers from the start.
    [[nodiscard]] static QStringList excludedApps();
    void setExcludedApps(const QStringList& apps);
    [[nodiscard]] static bool isExcluded(const QString& app);

    /// Keeps a finished run, unless history is off or the app is excluded; its id, or 0.
    qint64 record(const Entry& entry);
    /// Replaces a kept entry's result (another version, an edited copy).
    bool update(qint64 id, const QString& result);
    [[nodiscard]] QList<Entry> entries();
    bool remove(qint64 id);
    [[nodiscard]] Stats stats();
    /// Removes every entry and the file's key; history stays on for what comes next.
    bool deleteAll();
    /// Deletes entries past keepDays; at start and once a day.
    int prune();

Q_SIGNALS:
    /// Entries, the switch or the options changed.
    void changed();

private:
    Store* store();

    QString m_path;
    SecretFactory m_secrets;
    std::unique_ptr<SecretSource> m_source;
    std::unique_ptr<Store> m_store;
    Problem m_problem = Problem::None;
    QTimer* m_daily;
};

} // namespace paraphrase::app::history
