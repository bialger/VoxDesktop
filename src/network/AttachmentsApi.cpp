#include "network/AttachmentsApi.hpp"

#include "network/ApiPaths.hpp"
#include "network/JsonCodec.hpp"

#include <QUrlQuery>

namespace vox::network {

AttachmentsApi::AttachmentsApi(NetworkAccess &network) : m_network(network) {
}

ApiResult<AttachmentUploadInitResponse> AttachmentsApi::uploadInit(const AttachmentUploadInitRequest &request) const {
  const auto response = m_network.postJson(ApiPaths::kAttachmentsUploadInit, JsonCodec::toBytes(toJson(request)));

  ApiResult<AttachmentUploadInitResponse> result;
  result.statusCode = response.statusCode;

  const auto object = JsonCodec::parseObject(response.body, &result.error);
  if (!object.has_value()) {
    return result;
  }

  if (response.ok()) {
    result.data = parseAttachmentUploadInitResponse(*object);
    result.ok = result.data.has_value();
    if (!result.ok) {
      result.error = "Invalid attachment upload-init response";
    }
    return result;
  }

  const auto api_error = parseApiError(*object);
  result.error = api_error.has_value() ? api_error->message : response.errorMessage;
  return result;
}

VoidResult AttachmentsApi::uploadChunk(const QString &attachmentId,
                                       qint64 offset,
                                       QByteArrayView ciphertextChunk) const {
  QUrlQuery query;
  query.addQueryItem("offset", QString::number(offset));

  const auto response = m_network.putBytes(ApiPaths::kAttachmentsPrefix + attachmentId + "/chunk",
                                           QByteArray(ciphertextChunk.data(), ciphertextChunk.size()),
                                           query);

  VoidResult result;
  result.statusCode = response.statusCode;
  result.ok = response.ok();

  if (!result.ok) {
    const auto object = JsonCodec::parseObject(response.body, &result.error);
    if (object.has_value()) {
      const auto api_error = parseApiError(*object);
      if (api_error.has_value()) {
        result.error = api_error->message;
      }
    }
  }

  return result;
}

VoidResult AttachmentsApi::finalize(const QString &attachmentId, const QString &ciphertextHash) const {
  QJsonObject body;
  body.insert("ciphertext_hash", ciphertextHash);

  const auto response =
      m_network.postJson(ApiPaths::kAttachmentsPrefix + attachmentId + "/finalize", JsonCodec::toBytes(body));

  VoidResult result;
  result.statusCode = response.statusCode;
  result.ok = response.ok();

  if (!result.ok) {
    const auto object = JsonCodec::parseObject(response.body, &result.error);
    if (object.has_value()) {
      const auto api_error = parseApiError(*object);
      if (api_error.has_value()) {
        result.error = api_error->message;
      }
    }
  }

  return result;
}

ApiResult<QByteArray> AttachmentsApi::download(const QString &attachmentId) const {
  const auto response = m_network.get(ApiPaths::kAttachmentsPrefix + attachmentId);

  ApiResult<QByteArray> result;
  result.statusCode = response.statusCode;

  if (response.ok()) {
    result.ok = true;
    result.data = response.body;
    return result;
  }

  const auto object = JsonCodec::parseObject(response.body, &result.error);
  if (object.has_value()) {
    const auto api_error = parseApiError(*object);
    if (api_error.has_value()) {
      result.error = api_error->message;
    }
  }

  if (result.error.isEmpty()) {
    result.error = response.errorMessage;
  }

  return result;
}

} // namespace vox::network
