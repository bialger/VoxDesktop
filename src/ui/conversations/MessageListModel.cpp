#include "ui/conversations/MessageListModel.hpp"

namespace vox::ui::conversations {

MessageListModel::MessageListModel(QObject *parent) : QAbstractListModel(parent) {}

int MessageListModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_messages.size();
}

QVariant MessageListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.size()) {
        return {};
    }

    const auto &message = m_messages.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    {
        const auto ts = QDateTime::fromMSecsSinceEpoch(message.clientCreatedAtMs).toLocalTime();
        const QString stamp = (message.serverReceivedAtMs > 0) ? ts.toString("dd-MM-yyyy HH:mm") : ts.toString("HH:mm");
        return QString("[%1] %2").arg(stamp, QString::fromUtf8(message.plaintextCacheCiphertext));
    }
    case MessageIdRole:
        return message.messageId;
    case BodyRole:
        return QString::fromUtf8(message.plaintextCacheCiphertext);
    case AuthorRole:
        return message.senderUserId;
    case CiphertextRole:
        return QString::fromUtf8(message.ciphertextBlob.toBase64());
    case DeliveryStateRole:
        return domain::toString(message.deliveryState);
    case OutgoingRole:
        return message.isOutgoing;
    case TimestampRole:
        return QDateTime::fromMSecsSinceEpoch(message.clientCreatedAtMs);
    default:
        return {};
    }
}

QHash<int, QByteArray> MessageListModel::roleNames() const {
    return {{MessageIdRole, "message_id"},
            {BodyRole, "body"},
            {AuthorRole, "author"},
            {CiphertextRole, "ciphertext"},
            {DeliveryStateRole, "delivery_state"},
            {OutgoingRole, "outgoing"},
            {TimestampRole, "timestamp"}};
}

void MessageListModel::setMessages(QVector<domain::Message> messages) {
    beginResetModel();
    m_messages = std::move(messages);
    endResetModel();
}

} // namespace vox::ui::conversations
