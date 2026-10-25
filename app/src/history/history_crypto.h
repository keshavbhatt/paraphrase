#pragma once

#include <QByteArray>

#include <optional>

namespace paraphrase::app::history {

/// The history's sealing (ADR-017): AES-256-GCM with a random 96-bit nonce per field, under a
/// key derived from the desktop's secret. Nothing here touches the disk.

inline constexpr int kKeyBytes = 32;

/// HKDF-SHA256 of `secret`: the key the history is sealed with.
[[nodiscard]] QByteArray deriveKey(const QByteArray& secret);
/// nonce + ciphertext + tag; empty on a crypto failure.
[[nodiscard]] QByteArray seal(const QByteArray& key, const QByteArray& plain);
/// The plain bytes, or nothing when the key is wrong or the field was changed.
[[nodiscard]] std::optional<QByteArray> open(const QByteArray& key, const QByteArray& sealed);
/// `n` random bytes from the system's source.
[[nodiscard]] QByteArray randomBytes(int n);

} // namespace paraphrase::app::history
