#include "crypto/CryptoHelpers.hpp"

#include "crypto/SodiumInit.hpp"

#include <sodium.h>

namespace vox::crypto {
namespace {

bool checkLen(const QByteArrayView view, int expected) {
    return view.size() == expected;
}

QByteArray makeArray(size_t size) {
    return QByteArray(static_cast<int>(size), Qt::Uninitialized);
}

} // namespace

QByteArray CryptoHelpers::randomBytes(int size) {
    if (size <= 0 || !ensureSodiumInitialized()) {
        return {};
    }
    QByteArray out(size, Qt::Uninitialized);
    randombytes_buf(out.data(), static_cast<size_t>(out.size()));
    return out;
}

QByteArray CryptoHelpers::sha256(QByteArrayView data) {
    if (!ensureSodiumInitialized()) {
        return {};
    }

    QByteArray out(crypto_hash_sha256_BYTES, Qt::Uninitialized);
    crypto_hash_sha256(reinterpret_cast<unsigned char *>(out.data()),
                       reinterpret_cast<const unsigned char *>(data.data()),
                       static_cast<unsigned long long>(data.size()));
    return out;
}

QByteArray CryptoHelpers::hmacSha256(QByteArrayView key, QByteArrayView data) {
    if (!ensureSodiumInitialized() || key.empty()) {
        return {};
    }

    QByteArray out(crypto_auth_hmacsha256_BYTES, Qt::Uninitialized);
    crypto_auth_hmacsha256_state state;
    crypto_auth_hmacsha256_init(&state,
                                reinterpret_cast<const unsigned char *>(key.data()),
                                static_cast<size_t>(key.size()));
    crypto_auth_hmacsha256_update(&state,
                                  reinterpret_cast<const unsigned char *>(data.data()),
                                  static_cast<unsigned long long>(data.size()));
    crypto_auth_hmacsha256_final(&state, reinterpret_cast<unsigned char *>(out.data()));
    return out;
}

QByteArray CryptoHelpers::aeadEncrypt(QByteArrayView key,
                                      QByteArrayView nonce12,
                                      QByteArrayView plaintext,
                                      QByteArrayView ad) {
    if (!ensureSodiumInitialized() || !checkLen(key, crypto_aead_chacha20poly1305_ietf_KEYBYTES) ||
        !checkLen(nonce12, crypto_aead_chacha20poly1305_ietf_NPUBBYTES)) {
        return {};
    }

    QByteArray out = makeArray(static_cast<size_t>(plaintext.size()) +
                               crypto_aead_chacha20poly1305_ietf_ABYTES);
    unsigned long long outLen = 0;

    const auto rc = crypto_aead_chacha20poly1305_ietf_encrypt(
        reinterpret_cast<unsigned char *>(out.data()),
        &outLen,
        reinterpret_cast<const unsigned char *>(plaintext.data()),
        static_cast<unsigned long long>(plaintext.size()),
        reinterpret_cast<const unsigned char *>(ad.data()),
        static_cast<unsigned long long>(ad.size()),
        nullptr,
        reinterpret_cast<const unsigned char *>(nonce12.data()),
        reinterpret_cast<const unsigned char *>(key.data()));

    if (rc != 0) {
        return {};
    }

    out.resize(static_cast<int>(outLen));
    return out;
}

std::optional<QByteArray> CryptoHelpers::aeadDecrypt(QByteArrayView key,
                                                     QByteArrayView nonce12,
                                                     QByteArrayView ciphertext,
                                                     QByteArrayView ad) {
    if (!ensureSodiumInitialized() || !checkLen(key, crypto_aead_chacha20poly1305_ietf_KEYBYTES) ||
        !checkLen(nonce12, crypto_aead_chacha20poly1305_ietf_NPUBBYTES) ||
        ciphertext.size() < crypto_aead_chacha20poly1305_ietf_ABYTES) {
        return std::nullopt;
    }

    QByteArray out = makeArray(static_cast<size_t>(ciphertext.size()));
    unsigned long long outLen = 0;

    const auto rc = crypto_aead_chacha20poly1305_ietf_decrypt(
        reinterpret_cast<unsigned char *>(out.data()),
        &outLen,
        nullptr,
        reinterpret_cast<const unsigned char *>(ciphertext.data()),
        static_cast<unsigned long long>(ciphertext.size()),
        reinterpret_cast<const unsigned char *>(ad.data()),
        static_cast<unsigned long long>(ad.size()),
        reinterpret_cast<const unsigned char *>(nonce12.data()),
        reinterpret_cast<const unsigned char *>(key.data()));

    if (rc != 0) {
        return std::nullopt;
    }

    out.resize(static_cast<int>(outLen));
    return out;
}

QByteArray CryptoHelpers::xchachaEncryptLocal(QByteArrayView key,
                                              QByteArrayView nonce24,
                                              QByteArrayView plaintext,
                                              QByteArrayView ad) {
    if (!ensureSodiumInitialized() || !checkLen(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES) ||
        !checkLen(nonce24, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES)) {
        return {};
    }

    QByteArray out = makeArray(static_cast<size_t>(plaintext.size()) +
                               crypto_aead_xchacha20poly1305_ietf_ABYTES);
    unsigned long long outLen = 0;

    const auto rc = crypto_aead_xchacha20poly1305_ietf_encrypt(
        reinterpret_cast<unsigned char *>(out.data()),
        &outLen,
        reinterpret_cast<const unsigned char *>(plaintext.data()),
        static_cast<unsigned long long>(plaintext.size()),
        reinterpret_cast<const unsigned char *>(ad.data()),
        static_cast<unsigned long long>(ad.size()),
        nullptr,
        reinterpret_cast<const unsigned char *>(nonce24.data()),
        reinterpret_cast<const unsigned char *>(key.data()));

    if (rc != 0) {
        return {};
    }

    out.resize(static_cast<int>(outLen));
    return out;
}

std::optional<QByteArray> CryptoHelpers::xchachaDecryptLocal(QByteArrayView key,
                                                             QByteArrayView nonce24,
                                                             QByteArrayView ciphertext,
                                                             QByteArrayView ad) {
    if (!ensureSodiumInitialized() || !checkLen(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES) ||
        !checkLen(nonce24, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES) ||
        ciphertext.size() < crypto_aead_xchacha20poly1305_ietf_ABYTES) {
        return std::nullopt;
    }

    QByteArray out = makeArray(static_cast<size_t>(ciphertext.size()));
    unsigned long long outLen = 0;

    const auto rc = crypto_aead_xchacha20poly1305_ietf_decrypt(
        reinterpret_cast<unsigned char *>(out.data()),
        &outLen,
        nullptr,
        reinterpret_cast<const unsigned char *>(ciphertext.data()),
        static_cast<unsigned long long>(ciphertext.size()),
        reinterpret_cast<const unsigned char *>(ad.data()),
        static_cast<unsigned long long>(ad.size()),
        reinterpret_cast<const unsigned char *>(nonce24.data()),
        reinterpret_cast<const unsigned char *>(key.data()));

    if (rc != 0) {
        return std::nullopt;
    }

    out.resize(static_cast<int>(outLen));
    return out;
}

QByteArray CryptoHelpers::x25519(QByteArrayView priv32, QByteArrayView pub32) {
    if (!ensureSodiumInitialized() || !checkLen(priv32, crypto_scalarmult_SCALARBYTES) ||
        !checkLen(pub32, crypto_scalarmult_BYTES)) {
        return {};
    }

    QByteArray out(crypto_scalarmult_BYTES, Qt::Uninitialized);
    if (crypto_scalarmult(reinterpret_cast<unsigned char *>(out.data()),
                          reinterpret_cast<const unsigned char *>(priv32.data()),
                          reinterpret_cast<const unsigned char *>(pub32.data())) != 0) {
        return {};
    }

    return out;
}

QByteArray CryptoHelpers::ed25519Sign(QByteArrayView priv, QByteArrayView msg) {
    if (!ensureSodiumInitialized() || priv.size() != crypto_sign_SECRETKEYBYTES) {
        return {};
    }

    QByteArray sig(crypto_sign_BYTES, Qt::Uninitialized);
    unsigned long long sigLen = 0;
    const auto rc = crypto_sign_detached(reinterpret_cast<unsigned char *>(sig.data()),
                                         &sigLen,
                                         reinterpret_cast<const unsigned char *>(msg.data()),
                                         static_cast<unsigned long long>(msg.size()),
                                         reinterpret_cast<const unsigned char *>(priv.data()));
    if (rc != 0) {
        return {};
    }

    sig.resize(static_cast<int>(sigLen));
    return sig;
}

bool CryptoHelpers::ed25519Verify(QByteArrayView pub, QByteArrayView msg, QByteArrayView sig) {
    if (!ensureSodiumInitialized() || pub.size() != crypto_sign_PUBLICKEYBYTES ||
        sig.size() != crypto_sign_BYTES) {
        return false;
    }

    return crypto_sign_verify_detached(reinterpret_cast<const unsigned char *>(sig.data()),
                                       reinterpret_cast<const unsigned char *>(msg.data()),
                                       static_cast<unsigned long long>(msg.size()),
                                       reinterpret_cast<const unsigned char *>(pub.data())) == 0;
}

QByteArray CryptoHelpers::argon2idKdf(QByteArrayView password,
                                      QByteArrayView salt,
                                      quint64 opslimit,
                                      size_t memlimit,
                                      size_t outLen) {
    if (!ensureSodiumInitialized() || outLen == 0 ||
        salt.size() != crypto_pwhash_SALTBYTES || password.empty()) {
        return {};
    }

    QByteArray out = makeArray(outLen);
    const auto rc = crypto_pwhash(reinterpret_cast<unsigned char *>(out.data()),
                                  outLen,
                                  password.data(),
                                  static_cast<unsigned long long>(password.size()),
                                  reinterpret_cast<const unsigned char *>(salt.data()),
                                  static_cast<unsigned long long>(opslimit),
                                  memlimit,
                                  crypto_pwhash_ALG_ARGON2ID13);

    if (rc != 0) {
        return {};
    }

    return out;
}

} // namespace vox::crypto
