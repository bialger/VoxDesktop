#ifndef VOX_SERVICES_CONVERSATIONSERVICE_HPP
#define VOX_SERVICES_CONVERSATIONSERVICE_HPP

#include "network/ConversationsApi.hpp"
#include "network/DirectoryApi.hpp"
#include "services/Interfaces.hpp"
#include "storage/Repositories.hpp"

namespace vox::services {

class ConversationService final : public IConversationService {
public:
    ConversationService(network::ConversationsApi &api,
                        network::DirectoryApi &directoryApi,
                        storage::IConversationsRepository &conversationsRepository);

    bool refreshConversations() override;
    QVector<domain::Conversation> conversations() const override;

private:
    network::ConversationsApi &m_api;
    network::DirectoryApi &m_directoryApi;
    storage::IConversationsRepository &m_conversationsRepository;
};

} // namespace vox::services

#endif // VOX_SERVICES_CONVERSATIONSERVICE_HPP
