#ifndef VOX_SERVICES_MESSAGESENDSERVICE_HPP
#define VOX_SERVICES_MESSAGESENDSERVICE_HPP

#include "network/ConversationsApi.hpp"
#include "services/Interfaces.hpp"
#include "storage/Repositories.hpp"

namespace vox::services {

class MessageSendService final : public IMessageSendService {
public:
    MessageSendService(network::ConversationsApi &api,
                       storage::IMessagesRepository &messagesRepository,
                       storage::IJobsRepository &jobsRepository);

    bool sendCiphertext(const QString &conversationId,
                        const QString &deviceId,
                        const QString &ciphertext,
                        int envelopeType = 0) override;

    bool sendMessage(const QString &conversationId,
                     const QString &deviceId,
                     const QString &plaintext,
                     const QString &ciphertext,
                     int envelopeType = 0);

private:
    network::ConversationsApi &m_api;
    storage::IMessagesRepository &m_messagesRepository;
    storage::IJobsRepository &m_jobsRepository;
};

} // namespace vox::services

#endif // VOX_SERVICES_MESSAGESENDSERVICE_HPP
