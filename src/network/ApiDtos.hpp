#ifndef VOX_NETWORK_APIDTOS_HPP
#define VOX_NETWORK_APIDTOS_HPP

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include <optional>

namespace vox::network {

struct ApiError {
  int code{0};
  QString message;
};

struct RegisterRequest {
  QString username;
  QString passwordDerivedValue;
  QString deviceId;
  QString deviceLabel;
  QString identityKeyPublic;
  QString signedPrekeyPublic;
  QString signedPrekeySignature;
  QString wrappedSyncKey;
  QString syncWrapSalt;
  QJsonObject syncWrapParams;
};

struct LoginRequest {
  QString username;
  QString passwordDerivedValue;
  QString deviceId;
  QString deviceLabel;
  QString identityKeyPublic;
  QString signedPrekeyPublic;
  QString signedPrekeySignature;
};

struct AuthSessionResponse {
  QString userId;
  QString accessToken;
  QString refreshToken;
  QString deviceStatus;
  int syncKeyVersion{0};
};

struct RefreshRequest {
  QString refreshToken;
  QString deviceId;
};

struct RefreshResponse {
  QString accessToken;
  QString refreshToken;
};

struct MeResponse {
  QString userId;
  QString username;
  QString currentDeviceId;
  int syncKeyVersion{0};
};

struct DirectoryUser {
  QString userId;
  QString username;
};

struct DeviceDirectoryEntry {
  QString deviceId;
  QString deviceLabel;
  QString identityKeyPublic;
  QString signedPrekeyPublic;
  QString signedPrekeySignature;
};

struct ConversationCreateRequest {
  QString type;
  QString peerUserId;
  QVector<QString> members;
  QVector<QString> admins;
  QVector<QString> subscribers;
};

struct ConversationCreateResponse {
  QString conversationId;
};

struct ConversationSummaryDto {
  QString conversationId;
  int type{0};
  QString createdBy;
  QString createdByUsername;
  qint64 createdAt{0};
  int membershipVersion{0};
  QString myRole;
  QString title;
  QString channelPostPolicy;
  std::optional<QString> peerUserId;
};

struct ConversationMemberDto {
  QString userId;
  QString username;
  QString role;
};

struct ConversationMembersResponseDto {
  QString conversationId;
  int membershipVersion{0};
  QVector<ConversationMemberDto> members;
  QVector<ConversationMemberDto> admins;
  QVector<ConversationMemberDto> subscribers;
  QString subscriptionState;
  int memberCount{0};
};

struct SendEnvelopeRequest {
  QString deviceId;
  QString conversationId;
  QString ciphertext;
  QString envelopeId;
  int envelopeType{0};
  std::optional<int> orderingEpoch;
};

struct SendEnvelopeResponse {
  QString envelopeId;
  qint64 serverTimestamp{0};
  int deliveredToCount{0};
};

struct EnvelopeDto {
  QString envelopeId;
  QString conversationId;
  QString senderUserId;
  QString senderDeviceId;
  QString ciphertext;
  qint64 serverTimestamp{0};
  int envelopeType{0};
  std::optional<int> orderingEpoch;
};

struct EnvelopeBatchResponse {
  QVector<EnvelopeDto> envelopes;
  QString nextCursor;
  bool hasMore{false};
};

struct AttachmentUploadInitRequest {
  QString conversationId;
  qint64 fileSize{0};
  QString mimeHint;
};

struct AttachmentUploadInitResponse {
  QString attachmentId;
  QString blobPath;
};

struct SyncRecordDto {
  QString collection;
  QString recordId;
  int version{0};
  QString ciphertext;
  bool deleted{false};
};

struct SyncKeyBundleDto {
  int version{0};
  QString wrappedSyncKey;
  QString syncWrapSalt;
  QJsonObject syncWrapParams;
};

QJsonObject toJson(const RegisterRequest &req);
QJsonObject toJson(const LoginRequest &req);
QJsonObject toJson(const RefreshRequest &req);
QJsonObject toJson(const ConversationCreateRequest &req);
QJsonObject toJson(const SendEnvelopeRequest &req);
QJsonObject toJson(const AttachmentUploadInitRequest &req);
QJsonObject toJson(const SyncRecordDto &dto);

std::optional<ApiError> parseApiError(const QJsonObject &obj);
std::optional<AuthSessionResponse> parseAuthSessionResponse(const QJsonObject &obj);
std::optional<RefreshResponse> parseRefreshResponse(const QJsonObject &obj);
std::optional<MeResponse> parseMeResponse(const QJsonObject &obj);
std::optional<DirectoryUser> parseDirectoryUser(const QJsonObject &obj);
std::optional<DeviceDirectoryEntry> parseDeviceDirectoryEntry(const QJsonObject &obj);
std::optional<ConversationCreateResponse> parseConversationCreateResponse(const QJsonObject &obj);
std::optional<ConversationSummaryDto> parseConversationSummaryDto(const QJsonObject &obj);
std::optional<ConversationMemberDto> parseConversationMemberDto(const QJsonObject &obj);
std::optional<ConversationMembersResponseDto> parseConversationMembersResponseDto(const QJsonObject &obj);
std::optional<SendEnvelopeResponse> parseSendEnvelopeResponse(const QJsonObject &obj);
std::optional<EnvelopeDto> parseEnvelopeDto(const QJsonObject &obj);
std::optional<EnvelopeBatchResponse> parseEnvelopeBatchResponse(const QJsonObject &obj);
std::optional<AttachmentUploadInitResponse> parseAttachmentUploadInitResponse(const QJsonObject &obj);
std::optional<SyncRecordDto> parseSyncRecordDto(const QJsonObject &obj);
std::optional<SyncKeyBundleDto> parseSyncKeyBundleDto(const QJsonObject &obj);

} // namespace vox::network

#endif // VOX_NETWORK_APIDTOS_HPP
