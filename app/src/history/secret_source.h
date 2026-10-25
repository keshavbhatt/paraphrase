#pragma once

#include <QByteArray>
#include <QString>

#include <memory>
#include <optional>

namespace paraphrase::app::history {

/// Where the history's secret comes from (ADR-017): the desktop keeps it, the app never
/// writes it to a file.
class SecretSource
{
public:
    SecretSource() = default;
    virtual ~SecretSource() = default;
    SecretSource(const SecretSource&) = delete;
    SecretSource& operator=(const SecretSource&) = delete;
    SecretSource(SecretSource&&) = delete;
    SecretSource& operator=(SecretSource&&) = delete;

    /// The secret, made on first use; nothing when the desktop has no store or said no.
    [[nodiscard]] virtual std::optional<QByteArray> fetch() = 0;
    /// Drops the stored secret where the store allows it (the portal's cannot be dropped:
    /// the history's salt goes with its file instead).
    virtual void forget() = 0;
    /// Which store this is ("portal", "service"), kept so later runs ask the same one.
    [[nodiscard]] virtual QString kind() const = 0;
};

/// The store named `kind`, or when it is empty the Secret portal if the desktop offers it,
/// else the Secret Service; nothing when the one asked for (or any) is missing.
[[nodiscard]] std::unique_ptr<SecretSource> desktopSecretSource(const QString& kind = {});

} // namespace paraphrase::app::history
