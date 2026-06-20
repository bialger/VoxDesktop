#include "network/ApiDtos.hpp"

#include <QJsonArray>

namespace vox::network {
namespace {

std::optional<QString> GetString(const QJsonObject &obj, const char *field) {
  const auto value = obj.value(field);
  if (!value.isString()) {
    return std::nullopt;
  }
  return value.toString();
}

std::optional<qint64> GetInteger(const QJsonObject &obj, const char *field) {
  const auto value = obj.value(field);
  if (!value.isDouble()) {
    return std::nullopt;
  }
  return static_cast<qint64>(value.toInteger());
}

} // namespace

QJsonObject toJson(const RegisterRequest &req) {
  QJsonObject json;
  json.insert("username", req.username);
  json.insert("password_derived_value", req.passwordDerivedValue);
  json.insert("device_id", req.deviceId);
  json.insert("device_label", req.deviceLabel);
  json.insert("identity_key_public", req.identityKeyPublic);
  json.insert("signed_prekey_public", req.signedPrekeyPublic);
  json.insert("signed_prekey_signature", req.signedPrekeySignature);
  json.insert("wrapped_sync_key", req.wrappedSyncKey);
  json.insert("sync_wrap_salt", req.syncWrapSalt);
  json.insert("sync_wrap_params", req.syncWrapParams);
  return json;
}

QJsonObject toJson(const LoginRequest &req) {
  QJsonObject json;
  json.insert("username", req.username);
  json.insert("password_derived_value", req.passwordDerivedValue);
  json.insert("device_id", req.deviceId);
  if (!req.deviceLabel.isEmpty()) {
    json.insert("device_label", req.deviceLabel);
  }
  if (!req.identityKeyPublic.isEmpty()) {
    json.insert("identity_key_public", req.identityKeyPublic);
    json.insert("signed_prekey_public", req.signedPrekeyPublic);
    json.insert("signed_prekey_signature", req.signedPrekeySignature);
  }
  return json;
}

QJsonObject toJson(const RefreshRequest &req) {
  QJsonObject json;
  json.insert("refresh_token", req.refreshToken);
  json.insert("device_id", req.deviceId);
  return json;
}

QJsonObject toJson(const ConversationCreateRequest &req) {
  QJsonObject json;
  json.insert("type", req.type);

  if (req.type == "dm") {
    json.insert("peer_user_id", req.peerUserId);
  }
  if (!req.members.isEmpty()) {
    QJsonArray members;
    for (const auto &member : req.members) {
      members.push_back(member);
    }
    json.insert("members", members);
  }
  if (!req.admins.isEmpty()) {
    QJsonArray admins;
    for (const auto &admin : req.admins) {
      admins.push_back(admin);
    }
    json.insert("admins", admins);
  }
  if (!req.subscribers.isEmpty()) {
    QJsonArray subscribers;
    for (const auto &subscriber : req.subscribers) {
      subscribers.push_back(subscriber);
    }
    json.insert("subscribers", subscribers);
  }

  return json;
}

QJsonObject toJson(const SendEnvelopeRequest &req) {
  QJsonObject json;
  json.insert("device_id", req.deviceId);
  json.insert("conversation_id", req.conversationId);
  json.insert("ciphertext", req.ciphertext);
  json.insert("envelope_id", req.envelopeId);
  json.insert("envelope_type", req.envelopeType);
  if (req.orderingEpoch.has_value()) {
    json.insert("ordering_epoch", *req.orderingEpoch);
  }
  return json;
}

QJsonObject toJson(const AttachmentUploadInitRequest &req) {
  QJsonObject json;
  json.insert("conversation_id", req.conversationId);
  json.insert("file_size", static_cast<double>(req.fileSize));
  if (!req.mimeHint.isEmpty()) {
    json.insert("mime_hint", req.mimeHint);
  }
  return json;
}

QJsonObject toJson(const SyncRecordDto &dto) {
  QJsonObject json;
  json.insert("collection", dto.collection);
  json.insert("record_id", dto.recordId);
  json.insert("version", dto.version);
  json.insert("ciphertext", dto.ciphertext);
  json.insert("deleted", dto.deleted);
  return json;
}

std::optional<ApiError> parseApiError(const QJsonObject &obj) {
  const auto error_obj = obj.value("error").toObject();
  if (error_obj.isEmpty()) {
    return std::nullopt;
  }

  const auto code_value = error_obj.value("code");
  const auto msg_value = error_obj.value("message");
  if (!code_value.isDouble() || !msg_value.isString()) {
    return std::nullopt;
  }

  return ApiError{.code = code_value.toInt(), .message = msg_value.toString()};
}

std::optional<AuthSessionResponse> parseAuthSessionResponse(const QJsonObject &obj) {
  const auto user_id = GetString(obj, "user_id");
  const auto access_token = GetString(obj, "access_token");
  const auto refresh_token = GetString(obj, "refresh_token");
  const auto device_status = GetString(obj, "device_status");

  if (!user_id.has_value() || !access_token.has_value() || !refresh_token.has_value() || !device_status.has_value()) {
    return std::nullopt;
  }

  return AuthSessionResponse{.userId = *user_id,
                             .accessToken = *access_token,
                             .refreshToken = *refresh_token,
                             .deviceStatus = *device_status,
                             .syncKeyVersion = obj.value("sync_key_version").toInt()};
}

std::optional<RefreshResponse> parseRefreshResponse(const QJsonObject &obj) {
  const auto access_token = GetString(obj, "access_token");
  const auto refresh_token = GetString(obj, "refresh_token");
  if (!access_token.has_value() || !refresh_token.has_value()) {
    return std::nullopt;
  }

  return RefreshResponse{.accessToken = *access_token, .refreshToken = *refresh_token};
}

std::optional<MeResponse> parseMeResponse(const QJsonObject &obj) {
  const auto user_id = GetString(obj, "user_id");
  const auto username = GetString(obj, "username");
  const auto current_device = GetString(obj, "current_device_id");
  if (!user_id.has_value() || !username.has_value() || !current_device.has_value()) {
    return std::nullopt;
  }

  return MeResponse{.userId = *user_id,
                    .username = *username,
                    .currentDeviceId = *current_device,
                    .syncKeyVersion = obj.value("sync_key_version").toInt()};
}

std::optional<DirectoryUser> parseDirectoryUser(const QJsonObject &obj) {
  const auto user_id = GetString(obj, "user_id");
  const auto username = GetString(obj, "username");
  if (!user_id.has_value() || !username.has_value()) {
    return std::nullopt;
  }

  return DirectoryUser{.userId = *user_id, .username = *username};
}

std::optional<DeviceDirectoryEntry> parseDeviceDirectoryEntry(const QJsonObject &obj) {
  const auto device_id = GetString(obj, "device_id");
  const auto identity_key_public = GetString(obj, "identity_key_public");
  const auto signed_prekey_public = GetString(obj, "signed_prekey_public");
  const auto signed_prekey_signature = GetString(obj, "signed_prekey_signature");

  if (!device_id.has_value() || !identity_key_public.has_value() || !signed_prekey_public.has_value() ||
      !signed_prekey_signature.has_value()) {
    return std::nullopt;
  }

  return DeviceDirectoryEntry{.deviceId = *device_id,
                              .deviceLabel = obj.value("device_label").toString(),
                              .identityKeyPublic = *identity_key_public,
                              .signedPrekeyPublic = *signed_prekey_public,
                              .signedPrekeySignature = *signed_prekey_signature};
}

std::optional<ConversationCreateResponse> parseConversationCreateResponse(const QJsonObject &obj) {
  const auto conversation_id = GetString(obj, "conversation_id");
  if (!conversation_id.has_value()) {
    return std::nullopt;
  }
  return ConversationCreateResponse{.conversationId = *conversation_id};
}

std::optional<ConversationSummaryDto> parseConversationSummaryDto(const QJsonObject &obj) {
  const auto conversation_id = GetString(obj, "conversation_id");
  const auto created_by = GetString(obj, "created_by");
  const auto created_at = GetInteger(obj, "created_at");
  const auto membership_version = obj.value("membership_version");

  if (!conversation_id.has_value() || !created_by.has_value() || !created_at.has_value() ||
      !membership_version.isDouble()) {
    return std::nullopt;
  }

  ConversationSummaryDto dto;
  dto.conversationId = *conversation_id;
  dto.type = obj.value("type").toInt();
  dto.createdBy = *created_by;
  dto.createdByUsername = obj.value("created_by_username").toString();
  dto.createdAt = *created_at;
  dto.membershipVersion = membership_version.toInt();
  dto.myRole = obj.value("my_role").toString();
  dto.title = obj.value("title").toString();
  dto.channelPostPolicy = obj.value("channel_post_policy").toString();
  if (obj.contains("peer_user_id")) {
    dto.peerUserId = obj.value("peer_user_id").toString();
  }
  return dto;
}

std::optional<ConversationMemberDto> parseConversationMemberDto(const QJsonObject &obj) {
  const auto user_id = GetString(obj, "user_id");
  if (!user_id.has_value()) {
    return std::nullopt;
  }

  return ConversationMemberDto{
      .userId = *user_id, .username = obj.value("username").toString(), .role = obj.value("role").toString()};
}

std::optional<ConversationMembersResponseDto> parseConversationMembersResponseDto(const QJsonObject &obj) {
  const auto conversation_id = GetString(obj, "conversation_id");
  if (!conversation_id.has_value()) {
    return std::nullopt;
  }

  ConversationMembersResponseDto dto;
  dto.conversationId = *conversation_id;
  dto.membershipVersion = obj.value("membership_version").toInt();
  dto.subscriptionState = obj.value("subscription_state").toString();
  dto.memberCount = obj.value("member_count").toInt();

  const auto push_entries = [](const QJsonArray &array, QVector<ConversationMemberDto> &target) -> bool {
    for (const auto &entry : array) {
      const auto parsed = parseConversationMemberDto(entry.toObject());
      if (!parsed.has_value()) {
        return false;
      }
      target.push_back(*parsed);
    }
    return true;
  };

  if (!push_entries(obj.value("members").toArray(), dto.members) ||
      !push_entries(obj.value("admins").toArray(), dto.admins) ||
      !push_entries(obj.value("subscribers").toArray(), dto.subscribers)) {
    return std::nullopt;
  }

  return dto;
}

std::optional<SendEnvelopeResponse> parseSendEnvelopeResponse(const QJsonObject &obj) {
  const auto envelope_id = GetString(obj, "envelope_id");
  const auto server_timestamp = GetInteger(obj, "server_timestamp");
  if (!envelope_id.has_value() || !server_timestamp.has_value()) {
    return std::nullopt;
  }

  return SendEnvelopeResponse{.envelopeId = *envelope_id,
                              .serverTimestamp = *server_timestamp,
                              .deliveredToCount = obj.value("delivered_to_count").toInt()};
}

std::optional<EnvelopeDto> parseEnvelopeDto(const QJsonObject &obj) {
  const auto envelope_id = GetString(obj, "envelope_id");
  const auto conversation_id = GetString(obj, "conversation_id");
  const auto sender_user_id = GetString(obj, "sender_user_id");
  const auto sender_device_id = GetString(obj, "sender_device_id");
  const auto ciphertext = GetString(obj, "ciphertext");
  const auto server_timestamp = GetInteger(obj, "server_timestamp");

  if (!envelope_id.has_value() || !conversation_id.has_value() || !sender_user_id.has_value() ||
      !sender_device_id.has_value() || !ciphertext.has_value() || !server_timestamp.has_value()) {
    return std::nullopt;
  }

  EnvelopeDto dto;
  dto.envelopeId = *envelope_id;
  dto.conversationId = *conversation_id;
  dto.senderUserId = *sender_user_id;
  dto.senderDeviceId = *sender_device_id;
  dto.ciphertext = *ciphertext;
  dto.serverTimestamp = *server_timestamp;
  dto.envelopeType = obj.value("envelope_type").toInt();
  if (obj.contains("ordering_epoch") && obj.value("ordering_epoch").isDouble()) {
    dto.orderingEpoch = obj.value("ordering_epoch").toInt();
  }
  return dto;
}

std::optional<EnvelopeBatchResponse> parseEnvelopeBatchResponse(const QJsonObject &obj) {
  EnvelopeBatchResponse response;
  const auto envelopes = obj.value("envelopes").toArray();

  response.envelopes.reserve(envelopes.size());
  for (const auto &item : envelopes) {
    const auto parsed = parseEnvelopeDto(item.toObject());
    if (!parsed.has_value()) {
      return std::nullopt;
    }
    response.envelopes.push_back(*parsed);
  }

  response.nextCursor = obj.value("next_cursor").toString();
  response.hasMore = obj.value("has_more").toBool(false);
  return response;
}

std::optional<AttachmentUploadInitResponse> parseAttachmentUploadInitResponse(const QJsonObject &obj) {
  const auto attachment_id = GetString(obj, "attachment_id");
  const auto blob_path = GetString(obj, "blob_path");
  if (!attachment_id.has_value() || !blob_path.has_value()) {
    return std::nullopt;
  }

  return AttachmentUploadInitResponse{.attachmentId = *attachment_id, .blobPath = *blob_path};
}

std::optional<SyncRecordDto> parseSyncRecordDto(const QJsonObject &obj) {
  const auto collection = GetString(obj, "collection");
  const auto record_id = GetString(obj, "record_id");
  const auto ciphertext = GetString(obj, "ciphertext");
  if (!collection.has_value() || !record_id.has_value() || !ciphertext.has_value()) {
    return std::nullopt;
  }

  return SyncRecordDto{.collection = *collection,
                       .recordId = *record_id,
                       .version = obj.value("version").toInt(),
                       .ciphertext = *ciphertext,
                       .deleted = obj.value("deleted").toBool(false)};
}

std::optional<SyncKeyBundleDto> parseSyncKeyBundleDto(const QJsonObject &obj) {
  const auto wrapped_sync_key = GetString(obj, "wrapped_sync_key");
  const auto sync_wrap_salt = GetString(obj, "sync_wrap_salt");
  if (!wrapped_sync_key.has_value() || !sync_wrap_salt.has_value()) {
    return std::nullopt;
  }

  return SyncKeyBundleDto{.version = obj.value("version").toInt(),
                          .wrappedSyncKey = *wrapped_sync_key,
                          .syncWrapSalt = *sync_wrap_salt,
                          .syncWrapParams = obj.value("sync_wrap_params").toObject()};
}

} // namespace vox::network
