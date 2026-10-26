#pragma once

#include "backends.h"

#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QString>
#include <QVariantMap>

namespace paraphrase::app {

/// The global shortcut through the GlobalShortcuts portal (KDE Plasma, recent GNOME): one
/// session for the app's lifetime with one shortcut, "popup". The desktop shows the user the
/// shortcut and lets them change it in its settings; the default asked for is Ctrl+Alt+Space.
class PortalShortcut final : public ShortcutBackend
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(PortalShortcut)

public:
    static constexpr char kShortcutId[] = "popup";
    static constexpr int kBindTimeoutMs = 120'000; ///< the desktop may ask the user first

    explicit PortalShortcut(QObject* parent = nullptr);
    ~PortalShortcut() override;

    [[nodiscard]] QString name() const override;
    [[nodiscard]] bool available() const override { return m_available; }
    bool bind(const QKeySequence& keys) override;
    [[nodiscard]] QString description() const override;
    [[nodiscard]] QString keys() const override { return m_trigger; }
    /// Ends the portal session; the desktop keeps the user's choice for the next one.
    void close();

    /// "CTRL+ALT+space" for Ctrl+Alt+Space: the trigger format of the XDG shortcuts spec.
    [[nodiscard]] static QString preferredTrigger(const QKeySequence& keys);

private Q_SLOTS:
    void onActivated(const QDBusObjectPath& session, const QString& id, qulonglong timestamp,
                     const QVariantMap& options);
    /// The user changed the keys in the desktop's settings (ShortcutsChanged).
    void onShortcutsChanged(const QDBusMessage& message);

private:
    [[nodiscard]] bool createSession();
    void readTrigger(const QDBusArgument& arg);

    bool m_available = false;
    QString m_session;
    QString m_trigger; ///< what the desktop says the shortcut is, e.g. "Ctrl+Alt+Space"
};

} // namespace paraphrase::app
