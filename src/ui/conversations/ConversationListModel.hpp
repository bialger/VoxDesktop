#ifndef VOX_UI_CONVERSATIONS_CONVERSATIONLISTMODEL_HPP
#define VOX_UI_CONVERSATIONS_CONVERSATIONLISTMODEL_HPP

#include <QAbstractListModel>
#include <QVector>

#include "domain/Models.hpp"

namespace vox::ui::conversations {

class ConversationListModel final : public QAbstractListModel {
public:
    enum Role {
        ConversationIdRole = Qt::UserRole + 1,
        TitleRole,
        TypeRole,
        TypeLabelRole,
        UnreadCountRole,
        LastMessageAtRole,
        MutedRole
    };

    explicit ConversationListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setConversations(QVector<domain::Conversation> conversations);
    const QVector<domain::Conversation> &conversations() const;

private:
    QVector<domain::Conversation> m_conversations;
};

} // namespace vox::ui::conversations

#endif // VOX_UI_CONVERSATIONS_CONVERSATIONLISTMODEL_HPP
