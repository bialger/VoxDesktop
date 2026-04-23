#include "network/ConversationsApi.hpp"

#include "network/ApiPaths.hpp"
#include "network/JsonCodec.hpp"

#include <QJsonArray>
#include <QUrlQuery>

namespace vox::network {

ConversationsApi::ConversationsApi(NetworkAccess &network) : m_network(network) {
}

ApiResult<QVector<ConversationSummaryDto>> ConversationsApi::listConversations() const {
  const auto response = m_network.get(ApiPaths::kConversations);

  ApiResult<QVector<ConversationSummaryDto>> result;
  result.statusCode = response.statusCode;

  const auto object = JsonCodec::parseObject(response.body, &result.error);
  if (!object.has_value()) {
    return result;
  }

  if (response.ok()) {
    const auto array = object->value("conversations").toArray();
    QVector<ConversationSummaryDto> conversations;
    conversations.reserve(array.size());

    for (const auto &entry : array) {
      const auto parsed = parseConversationSummaryDto(entry.toObject());
      if (!parsed.has_value()) {
        result.error = "Invalid conversation entry";
        return result;
      }
      conversations.push_back(*parsed);
    }

    result.ok = true;
    result.data = conversations;
    return result;
  }

  const auto apiError = parseApiError(*object);
  result.error = apiError.has_value() ? apiError->message : response.errorMessage;
  return result;
}

ApiResult<ConversationSummaryDto> ConversationsApi::conversation(const QString &conversationId) const {
  const auto response = m_network.get(ApiPaths::kConversations + "/" + conversationId);

  ApiResult<ConversationSummaryDto> result;
  result.statusCode = response.statusCode;

  const auto object = JsonCodec::parseObject(response.body, &result.error);
  if (!object.has_value()) {
    return result;
  }

  if (response.ok()) {
    result.data = parseConversationSummaryDto(*object);
    result.ok = result.data.has_value();
    if (!result.ok) {
      result.error = "Invalid conversation payload";
    }
    return result;
  }

  const auto apiError = parseApiError(*object);
  result.error = apiError.has_value() ? apiError->message : response.errorMessage;
  return result;
}

ApiResult<ConversationMembersResponseDto> ConversationsApi::members(const QString &conversationId) const {
  const auto response = m_network.get(ApiPaths::kConversations + "/" + conversationId + "/members");

  ApiResult<ConversationMembersResponseDto> result;
  result.statusCode = response.statusCode;

  const auto object = JsonCodec::parseObject(response.body, &result.error);
  if (!object.has_value()) {
    return result;
  }

  if (response.ok()) {
    result.data = parseConversationMembersResponseDto(*object);
    result.ok = result.data.has_value();
    if (!result.ok) {
      result.error = "Invalid members payload";
    }
    return result;
  }

  const auto apiError = parseApiError(*object);
  result.error = apiError.has_value() ? apiError->message : response.errorMessage;
  return result;
}

ApiResult<ConversationCreateResponse> ConversationsApi::createConversation(
    const ConversationCreateRequest &request) const {
  const auto response = m_network.postJson(ApiPaths::kConversations, JsonCodec::toBytes(toJson(request)));

  ApiResult<ConversationCreateResponse> result;
  result.statusCode = response.statusCode;

  const auto object = JsonCodec::parseObject(response.body, &result.error);
  if (!object.has_value()) {
    return result;
  }

  if (response.ok()) {
    result.data = parseConversationCreateResponse(*object);
    result.ok = result.data.has_value();
    if (!result.ok) {
      result.error = "Invalid create conversation payload";
    }
    return result;
  }

  const auto apiError = parseApiError(*object);
  result.error = apiError.has_value() ? apiError->message : response.errorMessage;
  return result;
}

VoidResult ConversationsApi::addMember(const QString &conversationId,
                                       const QString &userId,
                                       const QString &role) const {
  QJsonObject body;
  body.insert("user_id", userId);
  body.insert("role", role);

  return parseVoid(
      m_network.postJson(ApiPaths::kConversations + "/" + conversationId + "/members", JsonCodec::toBytes(body)));
}

VoidResult ConversationsApi::removeMember(const QString &conversationId, const QString &userId) const {
  return parseVoid(m_network.deleteRequest(ApiPaths::kConversations + "/" + conversationId + "/members/" + userId));
}

VoidResult ConversationsApi::subscribe(const QString &conversationId) const {
  return parseVoid(
      m_network.postJson(ApiPaths::kConversations + "/" + conversationId + "/subscribe", JsonCodec::toBytes({})));
}

VoidResult ConversationsApi::unsubscribe(const QString &conversationId) const {
  return parseVoid(
      m_network.postJson(ApiPaths::kConversations + "/" + conversationId + "/unsubscribe", JsonCodec::toBytes({})));
}

ApiResult<SendEnvelopeResponse> ConversationsApi::sendEnvelope(const SendEnvelopeRequest &request) const {
  const auto response = m_network.postJson(ApiPaths::kMessagesSend, JsonCodec::toBytes(toJson(request)));

  ApiResult<SendEnvelopeResponse> result;
  result.statusCode = response.statusCode;

  const auto object = JsonCodec::parseObject(response.body, &result.error);
  if (!object.has_value()) {
    return result;
  }

  if (response.ok()) {
    result.data = parseSendEnvelopeResponse(*object);
    result.ok = result.data.has_value();
    if (!result.ok) {
      result.error = "Invalid send response";
    }
    return result;
  }

  const auto apiError = parseApiError(*object);
  result.error = apiError.has_value() ? apiError->message : response.errorMessage;
  return result;
}

VoidResult ConversationsApi::ackEnvelope(const QString &deviceId, const QString &envelopeId) const {
  QJsonObject body;
  body.insert("device_id", deviceId);
  body.insert("envelope_id", envelopeId);
  return parseVoid(m_network.postJson(ApiPaths::kMessagesAck, JsonCodec::toBytes(body)));
}

ApiResult<EnvelopeBatchResponse> ConversationsApi::pendingEnvelopes(int limit, const QString &cursor) const {
  QUrlQuery query;
  query.addQueryItem("limit", QString::number(limit));
  if (!cursor.isEmpty()) {
    query.addQueryItem("cursor", cursor);
  }

  const auto response = m_network.get(ApiPaths::kSyncPending, query);

  ApiResult<EnvelopeBatchResponse> result;
  result.statusCode = response.statusCode;

  const auto object = JsonCodec::parseObject(response.body, &result.error);
  if (!object.has_value()) {
    return result;
  }

  if (response.ok()) {
    result.data = parseEnvelopeBatchResponse(*object);
    result.ok = result.data.has_value();
    if (!result.ok) {
      result.error = "Invalid pending envelopes response";
    }
    return result;
  }

  const auto apiError = parseApiError(*object);
  result.error = apiError.has_value() ? apiError->message : response.errorMessage;
  return result;
}

ApiResult<EnvelopeBatchResponse> ConversationsApi::conversationEnvelopes(const QString &conversationId,
                                                                         int limit,
                                                                         const QString &cursor,
                                                                         std::optional<qint64> since) const {
  QUrlQuery query;
  query.addQueryItem("limit", QString::number(limit));
  if (!cursor.isEmpty()) {
    query.addQueryItem("cursor", cursor);
  } else if (since.has_value()) {
    query.addQueryItem("since", QString::number(*since));
  }

  const auto response = m_network.get(ApiPaths::kConversations + "/" + conversationId + "/envelopes", query);

  ApiResult<EnvelopeBatchResponse> result;
  result.statusCode = response.statusCode;

  const auto object = JsonCodec::parseObject(response.body, &result.error);
  if (!object.has_value()) {
    return result;
  }

  if (response.ok()) {
    result.data = parseEnvelopeBatchResponse(*object);
    result.ok = result.data.has_value();
    if (!result.ok) {
      result.error = "Invalid conversation envelopes response";
    }
    return result;
  }

  const auto apiError = parseApiError(*object);
  result.error = apiError.has_value() ? apiError->message : response.errorMessage;
  return result;
}

VoidResult ConversationsApi::parseVoid(const NetworkResponse &response) const {
  VoidResult result;
  result.statusCode = response.statusCode;
  result.ok = response.ok();

  if (result.ok) {
    return result;
  }

  const auto object = JsonCodec::parseObject(response.body, &result.error);
  if (object.has_value()) {
    const auto apiError = parseApiError(*object);
    if (apiError.has_value()) {
      result.error = apiError->message;
    }
  }
  if (result.error.isEmpty()) {
    result.error = response.errorMessage;
  }

  return result;
}

} // namespace vox::network
