#pragma once

#include "history/history_keeper.h"
#include "history/secret_source.h"

#include <QString>

#include <memory>

namespace paraphrase::testing {

/// A keyring for the history tests: always the same secret, nothing on the desktop.
class FakeSecrets : public app::history::SecretSource
{
public:
    std::optional<QByteArray> fetch() override { return QByteArray("a test secret"); }
    void forget() override {}
    [[nodiscard]] QString kind() const override { return QStringLiteral("fake"); }
};

/// A history in `path` with the fake keyring.
inline std::unique_ptr<app::history::Keeper> fakeHistory(const QString& path)
{
    return std::make_unique<app::history::Keeper>(
        path, [](const QString& /*kind*/) { return std::make_unique<FakeSecrets>(); });
}

} // namespace paraphrase::testing
