#ifndef VOX_SERVICES_INTERFACES_HPP
#define VOX_SERVICES_INTERFACES_HPP

#include <QByteArray>
#include <QVector>
#include <optional>

#include "domain/Models.hpp"

namespace vox::services {

struct AuthContext {
    QString userId;
    QString username;
    QString deviceId;
    QString accessToken;
    QString refreshToken;
};

class IAuthService {
public:
    virtual ~IAuthService() = default;
    virtual bool registerUser(const QString &username, const QString &passwordDerived) = 0;
    virtual bool login(const QString &username, const QString &passwordDerived) = 0;
    virtual bool restoreSession() = 0;
    virtual bool logout() = 0;
    virtual std::optional<AuthContext> context() const = 0;
};

class IConversationService {
public:
    virtual ~IConversationService() = default;
    virtual bool refreshConversations() = 0;
    virtual QVector<domain::Conversation> conversations() const = 0;
};

class IMessageSendService {
public:
    virtual ~IMessageSendService() = default;
    virtual bool sendCiphertext(const QString &conversationId,
                                const QString &deviceId,
                                const QString &ciphertext,
                                int envelopeType = 0) = 0;
};

class IAttachmentService {
public:
    virtual ~IAttachmentService() = default;
    virtual std::optional<QString> uploadEncrypted(const QString &conversationId,
                                                   QByteArrayView ciphertext,
                                                   const QString &mimeHint) = 0;
    virtual std::optional<QByteArray> downloadEncrypted(const QString &attachmentId) = 0;
};

class ISyncService {
public:
    virtual ~ISyncService() = default;
    virtual bool pullCollection(const QString &collection) = 0;
    virtual bool pushRecord(const domain::SyncRecord &record) = 0;
};

class ISearchService {
public:
    virtual ~ISearchService() = default;
    virtual bool indexMessage(const QString &conversationId,
                              const QString &messageId,
                              const QString &plaintext) = 0;
    virtual QVector<QString> findMessageIds(const QString &query) const = 0;
};

class INotificationService {
public:
    virtual ~INotificationService() = default;
    virtual QString notificationPreview(const QString &sender, const QString &message) const = 0;
};

} // namespace vox::services

#endif // VOX_SERVICES_INTERFACES_HPP
