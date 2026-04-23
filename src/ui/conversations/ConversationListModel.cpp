#include "ui/conversations/ConversationListModel.hpp"

namespace vox::ui::conversations {

ConversationListModel::ConversationListModel(QObject *parent) : QAbstractListModel(parent) {
}

int ConversationListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return m_conversations.size();
}

QVariant ConversationListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_conversations.size()) {
    return {};
  }

  const auto &conversation = m_conversations.at(index.row());
  switch (role) {
    case Qt::DisplayRole: {
      const QString stamp = conversation.lastMessageAt.toLocalTime().toString("dd-MM-yyyy HH:mm");
      return QString("%1  (%2)").arg(conversation.title, stamp);
    }
    case TitleRole:
      return conversation.title;
    case TypeRole:
      return domain::toString(conversation.type);
    case TypeLabelRole: {
      switch (conversation.type) {
        case domain::ConversationType::Dm:
          return QStringLiteral("DM");
        case domain::ConversationType::Group:
          return QStringLiteral("GROUP");
        case domain::ConversationType::Channel:
          return QStringLiteral("CHANNEL");
      }
      return QStringLiteral("DM");
    }
    case ConversationIdRole:
      return conversation.conversationId;
    case UnreadCountRole:
      return conversation.unreadCount;
    case LastMessageAtRole:
      return conversation.lastMessageAt;
    case MutedRole:
      return conversation.muted;
    default:
      return {};
  }
}

QHash<int, QByteArray> ConversationListModel::roleNames() const {
  return {{ConversationIdRole, "conversation_id"},
          {TitleRole, "title"},
          {TypeRole, "type"},
          {TypeLabelRole, "type_label"},
          {UnreadCountRole, "unread_count"},
          {LastMessageAtRole, "last_message_at"},
          {MutedRole, "muted"}};
}

void ConversationListModel::setConversations(QVector<domain::Conversation> conversations) {
  beginResetModel();
  m_conversations = std::move(conversations);
  endResetModel();
}

const QVector<domain::Conversation> &ConversationListModel::conversations() const {
  return m_conversations;
}

} // namespace vox::ui::conversations
