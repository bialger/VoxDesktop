#ifndef VOX_SERVICES_ATTACHMENTSERVICE_HPP
#define VOX_SERVICES_ATTACHMENTSERVICE_HPP

#include "network/AttachmentsApi.hpp"
#include "services/Interfaces.hpp"
#include "storage/FileStore.hpp"

namespace vox::services {

class AttachmentService final : public IAttachmentService {
public:
    AttachmentService(network::AttachmentsApi &api, storage::FileStore &fileStore);

    std::optional<QString> uploadEncrypted(const QString &conversationId,
                                           QByteArrayView ciphertext,
                                           const QString &mimeHint) override;

    std::optional<QByteArray> downloadEncrypted(const QString &attachmentId) override;

private:
    network::AttachmentsApi &m_api;
    storage::FileStore &m_fileStore;
};

} // namespace vox::services

#endif // VOX_SERVICES_ATTACHMENTSERVICE_HPP
