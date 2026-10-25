#include "history/history_crypto.h"

#include <algorithm>
#include <memory>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>

namespace paraphrase::app::history {
namespace {

constexpr int kNonceBytes = 12;
constexpr int kTagBytes = 16;
constexpr const char* kInfo = "paraphrase history v1";

using CipherCtx = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

unsigned char* bytes(QByteArray& b)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): OpenSSL takes unsigned bytes
    return reinterpret_cast<unsigned char*>(b.data());
}

const unsigned char* bytes(const QByteArray& b)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): OpenSSL takes unsigned bytes
    return reinterpret_cast<const unsigned char*>(b.constData());
}

} // namespace

QByteArray randomBytes(int n)
{
    QByteArray out(n, Qt::Uninitialized);
    if (RAND_bytes(bytes(out), n) != 1) {
        return {};
    }
    return out;
}

QByteArray deriveKey(const QByteArray& secret)
{
    const std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(
        EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr), &EVP_PKEY_CTX_free);
    QByteArray key(kKeyBytes, Qt::Uninitialized);
    auto length = static_cast<size_t>(kKeyBytes);
    const QByteArray info(kInfo);
    if (!ctx || secret.isEmpty() || EVP_PKEY_derive_init(ctx.get()) <= 0 ||
        EVP_PKEY_CTX_set_hkdf_md(ctx.get(), EVP_sha256()) <= 0 ||
        EVP_PKEY_CTX_set1_hkdf_key(ctx.get(), bytes(secret), static_cast<int>(secret.size())) <= 0 ||
        EVP_PKEY_CTX_add1_hkdf_info(ctx.get(), bytes(info), static_cast<int>(info.size())) <= 0 ||
        EVP_PKEY_derive(ctx.get(), bytes(key), &length) <= 0 || length != static_cast<size_t>(kKeyBytes)) {
        return {};
    }
    return key;
}

QByteArray seal(const QByteArray& key, const QByteArray& plain)
{
    if (key.size() != kKeyBytes) {
        return {};
    }
    const QByteArray nonce = randomBytes(kNonceBytes);
    const CipherCtx ctx(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    QByteArray out(kNonceBytes + plain.size() + kTagBytes, Qt::Uninitialized);
    int written = 0;
    int last = 0;
    unsigned char* body = bytes(out) + kNonceBytes;
    if (nonce.isEmpty() || !ctx ||
        EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, kNonceBytes, nullptr) != 1 ||
        EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, bytes(key), bytes(nonce)) != 1 ||
        EVP_EncryptUpdate(ctx.get(), body, &written, bytes(plain), static_cast<int>(plain.size())) != 1 ||
        EVP_EncryptFinal_ex(ctx.get(), body + written, &last) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, kTagBytes, body + written + last) != 1) {
        return {};
    }
    std::ranges::copy(nonce, out.begin());
    return out;
}

std::optional<QByteArray> open(const QByteArray& key, const QByteArray& sealed)
{
    if (key.size() != kKeyBytes || sealed.size() < kNonceBytes + kTagBytes) {
        return std::nullopt;
    }
    const auto plainSize = static_cast<int>(sealed.size()) - kNonceBytes - kTagBytes;
    QByteArray tag = sealed.right(kTagBytes);
    const CipherCtx ctx(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    QByteArray out(plainSize, Qt::Uninitialized);
    int written = 0;
    int last = 0;
    if (!ctx || EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, kNonceBytes, nullptr) != 1 ||
        EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, bytes(key), bytes(sealed)) != 1 ||
        EVP_DecryptUpdate(ctx.get(), bytes(out), &written, bytes(sealed) + kNonceBytes, plainSize) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, kTagBytes, tag.data()) != 1 ||
        EVP_DecryptFinal_ex(ctx.get(), bytes(out) + written, &last) != 1) {
        return std::nullopt; // a wrong key or a changed field
    }
    out.resize(written + last);
    return out;
}

} // namespace paraphrase::app::history
