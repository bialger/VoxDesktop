#include "services/ConversationService.hpp"

#include <QStringList>

namespace vox::services {

ConversationService::ConversationService(network::ConversationsApi &api,
                                         network::DirectoryApi &directoryApi,
                                         storage::IConversationsRepository &conversationsRepository) :
    m_api(api), m_directoryApi(directoryApi), m_conversationsRepository(conversationsRepository) {
}

bool ConversationService::refreshConversations() {
  const auto response = m_api.listConversations();
  if (!response.ok || !response.data.has_value()) {
    return false;
  }

  auto last6 = [](const QString &s) {
    if (s.size() <= 6)
      return s;
    return s.right(6);
  };

  for (const auto &dto : *response.data) {
    domain::Conversation conversation;
    conversation.conversationId = dto.conversationId;
    conversation.serverId = "default";
    conversation.type = static_cast<domain::ConversationType>(dto.type);
    QString title = dto.title;

    if (title.trimmed().isEmpty()) {
      // DM: resolve peer username if possible
      if (dto.type == 0 && dto.peerUserId.has_value() && !dto.peerUserId->isEmpty()) {
        const auto peer = m_directoryApi.userById(*dto.peerUserId);
        if (peer.ok && peer.data.has_value() && !peer.data->username.isEmpty()) {
          title = peer.data->username;
        } else {
          title = last6(*dto.peerUserId);
        }
      } else if (dto.type == 2) {
        // Channel: ask detail for title
        const auto detail = m_api.conversation(dto.conversationId);
        if (detail.ok && detail.data.has_value() && !detail.data->title.isEmpty()) {
          title = detail.data->title;
        } else {
          title = QString("Channel %1").arg(last6(dto.conversationId));
        }
      } else if (dto.type == 1) {
        // Group: build from member usernames
        const auto members = m_api.members(dto.conversationId);
        if (members.ok && members.data.has_value() && !members.data->members.isEmpty()) {
          QStringList names;
          for (const auto &m : members.data->members) {
            if (!m.username.isEmpty()) {
              names.push_back(m.username);
            }
            if (names.size() >= 3)
              break;
          }
          title = names.isEmpty() ? QString("Group %1").arg(last6(dto.conversationId)) : names.join(", ");
        } else {
          title = QString("Group %1").arg(last6(dto.conversationId));
        }
      } else {
        title = last6(dto.conversationId);
      }
    }

    conversation.title = title;
    conversation.lastMessageAt = QDateTime::fromSecsSinceEpoch(dto.createdAt);
    conversation.epoch = dto.membershipVersion;
    if (!m_conversationsRepository.upsertConversation(conversation)) {
      return false;
    }
  }

  return true;
}

QVector<domain::Conversation> ConversationService::conversations() const {
  return m_conversationsRepository.listConversations();
}

} // namespace vox::services
