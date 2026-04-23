#include "network/ApiDtos.hpp"

#include <QJsonArray>

namespace vox::network {
namespace {

std::optional<QString> getString(const QJsonObject &obj, const char *field) {
    const auto value = obj.value(field);
    if (!value.isString()) {
        return std::nullopt;
    }
    return value.toString();
}

std::optional<qint64> getInteger(const QJsonObject &obj, const char *field) {
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
    const auto errorObj = obj.value("error").toObject();
    if (errorObj.isEmpty()) {
        return std::nullopt;
    }

    const auto codeValue = errorObj.value("code");
    const auto msgValue = errorObj.value("message");
    if (!codeValue.isDouble() || !msgValue.isString()) {
        return std::nullopt;
    }

    return ApiError{codeValue.toInt(), msgValue.toString()};
}

std::optional<AuthSessionResponse> parseAuthSessionResponse(const QJsonObject &obj) {
    const auto userId = getString(obj, "user_id");
    const auto accessToken = getString(obj, "access_token");
    const auto refreshToken = getString(obj, "refresh_token");
    const auto deviceStatus = getString(obj, "device_status");

    if (!userId.has_value() || !accessToken.has_value() || !refreshToken.has_value() ||
        !deviceStatus.has_value()) {
        return std::nullopt;
    }

    return AuthSessionResponse{*userId,
                               *accessToken,
                               *refreshToken,
                               *deviceStatus,
                               obj.value("sync_key_version").toInt()};
}

std::optional<RefreshResponse> parseRefreshResponse(const QJsonObject &obj) {
    const auto accessToken = getString(obj, "access_token");
    const auto refreshToken = getString(obj, "refresh_token");
    if (!accessToken.has_value() || !refreshToken.has_value()) {
        return std::nullopt;
    }

    return RefreshResponse{*accessToken, *refreshToken};
}

std::optional<MeResponse> parseMeResponse(const QJsonObject &obj) {
    const auto userId = getString(obj, "user_id");
    const auto username = getString(obj, "username");
    const auto currentDevice = getString(obj, "current_device_id");
    if (!userId.has_value() || !username.has_value() || !currentDevice.has_value()) {
        return std::nullopt;
    }

    return MeResponse{*userId, *username, *currentDevice, obj.value("sync_key_version").toInt()};
}

std::optional<DirectoryUser> parseDirectoryUser(const QJsonObject &obj) {
    const auto userId = getString(obj, "user_id");
    const auto username = getString(obj, "username");
    if (!userId.has_value() || !username.has_value()) {
        return std::nullopt;
    }

    return DirectoryUser{*userId, *username};
}

std::optional<DeviceDirectoryEntry> parseDeviceDirectoryEntry(const QJsonObject &obj) {
    const auto deviceId = getString(obj, "device_id");
    const auto identityKeyPublic = getString(obj, "identity_key_public");
    const auto signedPrekeyPublic = getString(obj, "signed_prekey_public");
    const auto signedPrekeySignature = getString(obj, "signed_prekey_signature");

    if (!deviceId.has_value() || !identityKeyPublic.has_value() || !signedPrekeyPublic.has_value() ||
        !signedPrekeySignature.has_value()) {
        return std::nullopt;
    }

    return DeviceDirectoryEntry{*deviceId,
                                obj.value("device_label").toString(),
                                *identityKeyPublic,
                                *signedPrekeyPublic,
                                *signedPrekeySignature};
}

std::optional<ConversationCreateResponse> parseConversationCreateResponse(const QJsonObject &obj) {
    const auto conversationId = getString(obj, "conversation_id");
    if (!conversationId.has_value()) {
        return std::nullopt;
    }
    return ConversationCreateResponse{*conversationId};
}

std::optional<ConversationSummaryDto> parseConversationSummaryDto(const QJsonObject &obj) {
    const auto conversationId = getString(obj, "conversation_id");
    const auto createdBy = getString(obj, "created_by");
    const auto createdAt = getInteger(obj, "created_at");
    const auto membershipVersion = obj.value("membership_version");

    if (!conversationId.has_value() || !createdBy.has_value() || !createdAt.has_value() ||
        !membershipVersion.isDouble()) {
        return std::nullopt;
    }

    ConversationSummaryDto dto;
    dto.conversationId = *conversationId;
    dto.type = obj.value("type").toInt();
    dto.createdBy = *createdBy;
    dto.createdByUsername = obj.value("created_by_username").toString();
    dto.createdAt = *createdAt;
    dto.membershipVersion = membershipVersion.toInt();
    dto.myRole = obj.value("my_role").toString();
    dto.title = obj.value("title").toString();
    dto.channelPostPolicy = obj.value("channel_post_policy").toString();
    if (obj.contains("peer_user_id")) {
        dto.peerUserId = obj.value("peer_user_id").toString();
    }
    return dto;
}

std::optional<ConversationMemberDto> parseConversationMemberDto(const QJsonObject &obj) {
    const auto userId = getString(obj, "user_id");
    if (!userId.has_value()) {
        return std::nullopt;
    }

    return ConversationMemberDto{*userId, obj.value("username").toString(), obj.value("role").toString()};
}

std::optional<ConversationMembersResponseDto> parseConversationMembersResponseDto(const QJsonObject &obj) {
    const auto conversationId = getString(obj, "conversation_id");
    if (!conversationId.has_value()) {
        return std::nullopt;
    }

    ConversationMembersResponseDto dto;
    dto.conversationId = *conversationId;
    dto.membershipVersion = obj.value("membership_version").toInt();
    dto.subscriptionState = obj.value("subscription_state").toString();
    dto.memberCount = obj.value("member_count").toInt();

    const auto pushEntries = [](const QJsonArray &array, QVector<ConversationMemberDto> &target) -> bool {
        for (const auto &entry : array) {
            const auto parsed = parseConversationMemberDto(entry.toObject());
            if (!parsed.has_value()) {
                return false;
            }
            target.push_back(*parsed);
        }
        return true;
    };

    if (!pushEntries(obj.value("members").toArray(), dto.members) ||
        !pushEntries(obj.value("admins").toArray(), dto.admins) ||
        !pushEntries(obj.value("subscribers").toArray(), dto.subscribers)) {
        return std::nullopt;
    }

    return dto;
}

std::optional<SendEnvelopeResponse> parseSendEnvelopeResponse(const QJsonObject &obj) {
    const auto envelopeId = getString(obj, "envelope_id");
    const auto serverTimestamp = getInteger(obj, "server_timestamp");
    if (!envelopeId.has_value() || !serverTimestamp.has_value()) {
        return std::nullopt;
    }

    return SendEnvelopeResponse{*envelopeId, *serverTimestamp, obj.value("delivered_to_count").toInt()};
}

std::optional<EnvelopeDto> parseEnvelopeDto(const QJsonObject &obj) {
    const auto envelopeId = getString(obj, "envelope_id");
    const auto conversationId = getString(obj, "conversation_id");
    const auto senderUserId = getString(obj, "sender_user_id");
    const auto senderDeviceId = getString(obj, "sender_device_id");
    const auto ciphertext = getString(obj, "ciphertext");
    const auto serverTimestamp = getInteger(obj, "server_timestamp");

    if (!envelopeId.has_value() || !conversationId.has_value() || !senderUserId.has_value() ||
        !senderDeviceId.has_value() || !ciphertext.has_value() || !serverTimestamp.has_value()) {
        return std::nullopt;
    }

    EnvelopeDto dto;
    dto.envelopeId = *envelopeId;
    dto.conversationId = *conversationId;
    dto.senderUserId = *senderUserId;
    dto.senderDeviceId = *senderDeviceId;
    dto.ciphertext = *ciphertext;
    dto.serverTimestamp = *serverTimestamp;
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
    const auto attachmentId = getString(obj, "attachment_id");
    const auto blobPath = getString(obj, "blob_path");
    if (!attachmentId.has_value() || !blobPath.has_value()) {
        return std::nullopt;
    }

    return AttachmentUploadInitResponse{*attachmentId, *blobPath};
}

std::optional<SyncRecordDto> parseSyncRecordDto(const QJsonObject &obj) {
    const auto collection = getString(obj, "collection");
    const auto recordId = getString(obj, "record_id");
    const auto ciphertext = getString(obj, "ciphertext");
    if (!collection.has_value() || !recordId.has_value() || !ciphertext.has_value()) {
        return std::nullopt;
    }

    return SyncRecordDto{*collection,
                         *recordId,
                         obj.value("version").toInt(),
                         *ciphertext,
                         obj.value("deleted").toBool(false)};
}

std::optional<SyncKeyBundleDto> parseSyncKeyBundleDto(const QJsonObject &obj) {
    const auto wrappedSyncKey = getString(obj, "wrapped_sync_key");
    const auto syncWrapSalt = getString(obj, "sync_wrap_salt");
    if (!wrappedSyncKey.has_value() || !syncWrapSalt.has_value()) {
        return std::nullopt;
    }

    return SyncKeyBundleDto{obj.value("version").toInt(),
                            *wrappedSyncKey,
                            *syncWrapSalt,
                            obj.value("sync_wrap_params").toObject()};
}

} // namespace vox::network
