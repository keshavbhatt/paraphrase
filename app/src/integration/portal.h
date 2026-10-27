#pragma once

#include <QDBusObjectPath>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <optional>

namespace paraphrase::app::portal {

inline constexpr char kService[] = "org.freedesktop.portal.Desktop";
inline constexpr char kPath[] = "/org/freedesktop/portal/desktop";

/// True when the desktop portal offers `interface` (asks for its version property).
[[nodiscard]] bool hasInterface(const QString& interface);

/// The version of `interface` the desktop portal offers, or 0 when it has none.
[[nodiscard]] uint version(const QString& interface);

/// Tells the portal which app this host process is (org.freedesktop.host.portal.Registry),
/// once per process; KDE's portals need it for apps that are not sandboxed.
void registerApp();

/// Calls a portal method that answers through a Request object and waits for its Response
/// (the portal pattern): `args` end with the options map, which gets a handle_token. The
/// results, or nothing when the user cancelled, the call failed or `timeoutMs` passed.
/// `response`, when given, gets the Response code (0 done, 1 the user cancelled, 2 another
/// error) or kNoResponse when there was no answer at all.
[[nodiscard]] std::optional<QVariantMap> request(const QString& interface, const QString& method,
                                                 QVariantList args, int timeoutMs, int* response = nullptr);

/// request()'s `response` when the call failed or timed out.
inline constexpr int kNoResponse = -1;
/// request()'s `response` when the user said no in the desktop's dialog.
inline constexpr int kCancelled = 1;

/// A fresh token for handle_token and session_handle_token.
[[nodiscard]] QString token();

/// Receives one Request's Response signal.
class ResponseCatcher : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ResponseCatcher)

public:
    ResponseCatcher() = default;
    ~ResponseCatcher() override = default;

    std::optional<uint> code;
    QVariantMap results;

public Q_SLOTS:
    void onResponse(uint response, const QVariantMap& answer);

Q_SIGNALS:
    void received();
};

} // namespace paraphrase::app::portal
