#ifndef VOX_CRYPTO_CRYPTOHELPERS_HPP
#define VOX_CRYPTO_CRYPTOHELPERS_HPP

#include <QByteArray>
#include <QByteArrayView>
#include <optional>

namespace vox::crypto {

class CryptoHelpers final {
public:
    static QByteArray randomBytes(int size);

    static QByteArray sha256(QByteArrayView data);
    static QByteArray hmacSha256(QByteArrayView key, QByteArrayView data);

    static QByteArray aeadEncrypt(QByteArrayView key,
                                  QByteArrayView nonce12,
                                  QByteArrayView plaintext,
                                  QByteArrayView ad);

    static std::optional<QByteArray> aeadDecrypt(QByteArrayView key,
                                                 QByteArrayView nonce12,
                                                 QByteArrayView ciphertext,
                                                 QByteArrayView ad);

    static QByteArray xchachaEncryptLocal(QByteArrayView key,
                                          QByteArrayView nonce24,
                                          QByteArrayView plaintext,
                                          QByteArrayView ad);

    static std::optional<QByteArray> xchachaDecryptLocal(QByteArrayView key,
                                                         QByteArrayView nonce24,
                                                         QByteArrayView ciphertext,
                                                         QByteArrayView ad);

    static QByteArray x25519(QByteArrayView priv32, QByteArrayView pub32);
    static QByteArray ed25519Sign(QByteArrayView priv, QByteArrayView msg);
    static bool ed25519Verify(QByteArrayView pub, QByteArrayView msg, QByteArrayView sig);

    static QByteArray argon2idKdf(QByteArrayView password,
                                  QByteArrayView salt,
                                  quint64 opslimit,
                                  size_t memlimit,
                                  size_t outLen);
};

} // namespace vox::crypto

#endif // VOX_CRYPTO_CRYPTOHELPERS_HPP
