#ifndef VOX_NETWORK_ATTACHMENTSAPI_HPP
#define VOX_NETWORK_ATTACHMENTSAPI_HPP

#include "network/ApiDtos.hpp"
#include "network/ApiResult.hpp"
#include "network/NetworkAccess.hpp"

namespace vox::network {

class AttachmentsApi final {
public:
  explicit AttachmentsApi(NetworkAccess &network);

  ApiResult<AttachmentUploadInitResponse> uploadInit(const AttachmentUploadInitRequest &request) const;
  VoidResult uploadChunk(const QString &attachmentId, qint64 offset, QByteArrayView ciphertextChunk) const;
  VoidResult finalize(const QString &attachmentId, const QString &ciphertextHash) const;
  ApiResult<QByteArray> download(const QString &attachmentId) const;

private:
  NetworkAccess &m_network;
};

} // namespace vox::network

#endif // VOX_NETWORK_ATTACHMENTSAPI_HPP
