#ifndef VOX_DOMAIN_MODELS_HPP
#define VOX_DOMAIN_MODELS_HPP

#include <QString>
#include <QDateTime>
#include <QByteArray>
#include <optional>

namespace vox::domain {

enum class ConversationType { Dm, Group, Channel };
enum class DeliveryState {
    Draft,
    Queued,
    Encrypting,
    UploadingAttachments,
    Sending,
    SentToServer,
    DeliveredToPeerDevices,
    ReadLocal,
    Failed
};
enum class TrustLevel { Unknown, Unverified, Verified, Blocked };

enum class ContentKind { Text, Attachment, System };

struct ServerProfile {
    QString serverId;
    QString baseUrl;
    QString displayName;
    QString tlsMode{"strict"};
    QString pinnedCertSha256;
};

struct Account {
    QString accountId;
    QString serverId;
    QString userId;
    QString username;
    QString activeDeviceId;
    QByteArray refreshTokenCiphertext;
    QDateTime lastLogin;
};

struct Device {
    QString deviceId;
    QString userId;
    QString serverId;
    QString label;
    QByteArray identitySignPub;
    QByteArray identityDhPub;
    TrustLevel trustLevel{TrustLevel::Unknown};
    QDateTime lastSeen;
};

struct Conversation {
    QString conversationId;
    QString serverId;
    ConversationType type{ConversationType::Dm};
    QString title;
    QDateTime lastMessageAt;
    bool muted{false};
    int pinRank{0};
    int epoch{0};
    int unreadCount{0};
};

struct Message {
    QString messageId;
    QString conversationId;
    QString senderUserId;
    QString senderDeviceId;
    qint64 serverSeq{0};
    qint64 clientCreatedAtMs{0};
    qint64 serverReceivedAtMs{0};
    ContentKind contentKind{ContentKind::Text};
    QByteArray ciphertextBlob;
    QByteArray plaintextCacheCiphertext;
    DeliveryState deliveryState{DeliveryState::Draft};
    bool isOutgoing{false};
    std::optional<QString> replyToMessageId;
    int senderKeyEpoch{0};
};

struct Attachment {
    QString attachmentId;
    QString messageId;
    QString ciphertextPath;
    QByteArray mimeTypeCiphertext;
    QByteArray fileNameCiphertext;
    qint64 plaintextSize{0};
    qint64 ciphertextSize{0};
    QByteArray sha256Ciphertext;
};

struct SyncRecord {
    QString collection;
    QString recordId;
    int version{1};
    QByteArray ciphertext;
    qint64 updatedAtMs{0};
    bool tombstone{false};
};

struct DeviceFingerprint {
    QString deviceId;
    QString fingerprint;
    TrustLevel trustLevel{TrustLevel::Unknown};
    qint64 verifiedAtMs{0};
};

QString toString(DeliveryState state);
QString toString(ConversationType type);
QString toString(TrustLevel state);
QString toString(ContentKind kind);

std::optional<DeliveryState> deliveryStateFromString(const QString &state);
std::optional<ConversationType> conversationTypeFromString(const QString &type);
std::optional<TrustLevel> trustLevelFromString(const QString &state);
std::optional<ContentKind> contentKindFromString(const QString &kind);

} // namespace vox::domain

#endif // VOX_DOMAIN_MODELS_HPP
