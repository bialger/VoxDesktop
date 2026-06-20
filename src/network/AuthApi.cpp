#include "network/AuthApi.hpp"

#include "network/ApiPaths.hpp"
#include "network/JsonCodec.hpp"

namespace vox::network {

AuthApi::AuthApi(NetworkAccess &network) : m_network(network) {
}

ApiResult<AuthSessionResponse> AuthApi::registerUser(const RegisterRequest &request) const {
  const auto response = m_network.postJson(ApiPaths::kRegister, JsonCodec::toBytes(toJson(request)));
  return parseTypedResponse<AuthSessionResponse>(response, parseAuthSessionResponse);
}

ApiResult<AuthSessionResponse> AuthApi::login(const LoginRequest &request) const {
  const auto response = m_network.postJson(ApiPaths::kLogin, JsonCodec::toBytes(toJson(request)));
  return parseTypedResponse<AuthSessionResponse>(response, parseAuthSessionResponse);
}

ApiResult<RefreshResponse> AuthApi::refresh(const RefreshRequest &request) const {
  const auto response = m_network.postJson(ApiPaths::kRefresh, JsonCodec::toBytes(toJson(request)));
  return parseTypedResponse<RefreshResponse>(response, parseRefreshResponse);
}

ApiResult<MeResponse> AuthApi::me() const {
  const auto response = m_network.get(ApiPaths::kMe);
  return parseTypedResponse<MeResponse>(response, parseMeResponse);
}

VoidResult AuthApi::logout() const {
  const auto response = m_network.postJson(ApiPaths::kLogout, JsonCodec::toBytes(QJsonObject{}));
  return parseVoidResponse(response);
}

ApiResult<int> AuthApi::changePassword(const QString &currentPasswordDerived,
                                       const QString &newPasswordDerived,
                                       const QString &wrappedSyncKey,
                                       const QString &syncWrapSalt,
                                       const QJsonObject &syncWrapParams) const {
  QJsonObject body;
  body.insert("current_password_derived_value", currentPasswordDerived);
  body.insert("new_password_derived_value", newPasswordDerived);
  body.insert("wrapped_sync_key", wrappedSyncKey);
  body.insert("sync_wrap_salt", syncWrapSalt);
  body.insert("sync_wrap_params", syncWrapParams);

  const auto response = m_network.postJson(ApiPaths::kChangePassword, JsonCodec::toBytes(body));

  ApiResult<int> result;
  result.statusCode = response.statusCode;

  const auto object = JsonCodec::parseObject(response.body, &result.error);
  if (!object.has_value()) {
    return result;
  }

  if (response.ok()) {
    result.ok = object->value("sync_key_version").isDouble();
    if (result.ok) {
      result.data = object->value("sync_key_version").toInt();
    } else {
      result.error = "Missing sync_key_version";
    }
    return result;
  }

  const auto api_error = parseApiError(*object);
  result.error = api_error.has_value() ? api_error->message : response.errorMessage;
  return result;
}

VoidResult AuthApi::parseVoidResponse(const NetworkResponse &response) const {
  VoidResult result;
  result.statusCode = response.statusCode;
  result.ok = response.ok();

  if (result.ok) {
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
