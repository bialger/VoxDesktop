#ifndef VOX_CRYPTO_ATTACHMENTCRYPTO_HPP
#define VOX_CRYPTO_ATTACHMENTCRYPTO_HPP

#include <QByteArray>
#include <QByteArrayView>
#include <optional>

namespace vox::crypto {

struct EncryptedAttachmentChunk {
  QByteArray nonce;
  QByteArray ciphertext;
  QByteArray sha256;
};

class AttachmentCrypto final {
public:
  static std::optional<EncryptedAttachmentChunk> encryptChunk(QByteArrayView key32,
                                                              QByteArrayView plaintext,
                                                              QByteArrayView ad);

  static std::optional<QByteArray> decryptChunk(QByteArrayView key32,
                                                QByteArrayView nonce,
                                                QByteArrayView ciphertext,
                                                QByteArrayView ad);
};

} // namespace vox::crypto

#endif // VOX_CRYPTO_ATTACHMENTCRYPTO_HPP
