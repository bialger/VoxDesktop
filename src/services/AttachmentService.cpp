#include "services/AttachmentService.hpp"

#include "crypto/CryptoHelpers.hpp"

namespace vox::services {

AttachmentService::AttachmentService(network::AttachmentsApi &api, storage::FileStore &fileStore) :
    m_api(api), m_fileStore(fileStore) {
}

std::optional<QString> AttachmentService::uploadEncrypted(const QString &conversationId,
                                                          QByteArrayView ciphertext,
                                                          const QString &mimeHint) {
  if (conversationId.isEmpty() || ciphertext.empty()) {
    return std::nullopt;
  }

  const network::AttachmentUploadInitRequest request{conversationId, static_cast<qint64>(ciphertext.size()), mimeHint};
  const auto init = m_api.uploadInit(request);
  if (!init.ok || !init.data.has_value()) {
    return std::nullopt;
  }

  if (!m_api.uploadChunk(init.data->attachmentId, 0, ciphertext).ok) {
    return std::nullopt;
  }

  const auto hash = QString("sha256:%1").arg(QString::fromUtf8(crypto::CryptoHelpers::sha256(ciphertext).toHex()));
  if (!m_api.finalize(init.data->attachmentId, hash).ok) {
    return std::nullopt;
  }

  if (!m_fileStore.writeEncryptedAttachment(init.data->attachmentId, ciphertext)) {
    return std::nullopt;
  }

  return init.data->attachmentId;
}

std::optional<QByteArray> AttachmentService::downloadEncrypted(const QString &attachmentId) {
  if (const auto local = m_fileStore.readEncryptedAttachment(attachmentId); local.has_value()) {
    return local;
  }

  const auto downloaded = m_api.download(attachmentId);
  if (!downloaded.ok || !downloaded.data.has_value()) {
    return std::nullopt;
  }

  if (!m_fileStore.writeEncryptedAttachment(attachmentId, *downloaded.data)) {
    return std::nullopt;
  }

  return downloaded.data;
}

} // namespace vox::services
