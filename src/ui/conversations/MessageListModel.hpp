#ifndef VOX_UI_CONVERSATIONS_MESSAGELISTMODEL_HPP
#define VOX_UI_CONVERSATIONS_MESSAGELISTMODEL_HPP

#include <QAbstractListModel>
#include <QVector>

#include "domain/Models.hpp"

namespace vox::ui::conversations {

class MessageListModel final : public QAbstractListModel {
public:
    enum Role {
        MessageIdRole = Qt::UserRole + 1,
        BodyRole,
        AuthorRole,
        CiphertextRole,
        DeliveryStateRole,
        OutgoingRole,
        TimestampRole
    };

    explicit MessageListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setMessages(QVector<domain::Message> messages);

private:
    QVector<domain::Message> m_messages;
};

} // namespace vox::ui::conversations

#endif // VOX_UI_CONVERSATIONS_MESSAGELISTMODEL_HPP
