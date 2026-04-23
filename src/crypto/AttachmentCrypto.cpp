#include "crypto/AttachmentCrypto.hpp"

#include "crypto/CryptoHelpers.hpp"

namespace vox::crypto {

std::optional<EncryptedAttachmentChunk> AttachmentCrypto::encryptChunk(QByteArrayView key32,
                                                                       QByteArrayView plaintext,
                                                                       QByteArrayView ad) {
    if (key32.size() != 32) {
        return std::nullopt;
    }

    const QByteArray nonce = CryptoHelpers::randomBytes(12);
    if (nonce.size() != 12) {
        return std::nullopt;
    }

    const QByteArray ciphertext = CryptoHelpers::aeadEncrypt(key32, nonce, plaintext, ad);
    if (ciphertext.isEmpty()) {
        return std::nullopt;
    }

    const QByteArray digest = CryptoHelpers::sha256(ciphertext);
    return EncryptedAttachmentChunk{nonce, ciphertext, digest};
}

std::optional<QByteArray> AttachmentCrypto::decryptChunk(QByteArrayView key32,
                                                         QByteArrayView nonce,
                                                         QByteArrayView ciphertext,
                                                         QByteArrayView ad) {
    return CryptoHelpers::aeadDecrypt(key32, nonce, ciphertext, ad);
}

} // namespace vox::crypto
