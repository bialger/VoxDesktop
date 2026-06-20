#ifndef VOX_NETWORK_CONVERSATIONSAPI_HPP
#define VOX_NETWORK_CONVERSATIONSAPI_HPP

#include "network/ApiDtos.hpp"
#include "network/ApiResult.hpp"
#include "network/NetworkAccess.hpp"

namespace vox::network {

class ConversationsApi final {
public:
  explicit ConversationsApi(NetworkAccess &network);

  ApiResult<QVector<ConversationSummaryDto>> listConversations() const;
  ApiResult<ConversationSummaryDto> conversation(const QString &conversationId) const;
  ApiResult<ConversationMembersResponseDto> members(const QString &conversationId) const;

  ApiResult<ConversationCreateResponse> createConversation(const ConversationCreateRequest &request) const;
  VoidResult addMember(const QString &conversationId, const QString &userId, const QString &role) const;
  VoidResult removeMember(const QString &conversationId, const QString &userId) const;
  VoidResult subscribe(const QString &conversationId) const;
  VoidResult unsubscribe(const QString &conversationId) const;

  ApiResult<SendEnvelopeResponse> sendEnvelope(const SendEnvelopeRequest &request) const;
  VoidResult ackEnvelope(const QString &deviceId, const QString &envelopeId) const;

  ApiResult<EnvelopeBatchResponse> pendingEnvelopes(int limit, const QString &cursor = {}) const;
  ApiResult<EnvelopeBatchResponse> conversationEnvelopes(const QString &conversationId,
                                                         int limit,
                                                         const QString &cursor = {},
                                                         std::optional<qint64> since = std::nullopt) const;

private:
  VoidResult parseVoid(const NetworkResponse &response) const;

  NetworkAccess &m_network;
};

} // namespace vox::network

#endif // VOX_NETWORK_CONVERSATIONSAPI_HPP
