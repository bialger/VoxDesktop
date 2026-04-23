#ifndef VOX_NETWORK_AUTHAPI_HPP
#define VOX_NETWORK_AUTHAPI_HPP

#include "network/ApiDtos.hpp"
#include "network/ApiResult.hpp"
#include "network/JsonCodec.hpp"
#include "network/NetworkAccess.hpp"

namespace vox::network {

class AuthApi final {
public:
  explicit AuthApi(NetworkAccess &network);

  ApiResult<AuthSessionResponse> registerUser(const RegisterRequest &request) const;
  ApiResult<AuthSessionResponse> login(const LoginRequest &request) const;
  ApiResult<RefreshResponse> refresh(const RefreshRequest &request) const;
  ApiResult<MeResponse> me() const;
  VoidResult logout() const;
  ApiResult<int> changePassword(const QString &currentPasswordDerived,
                                const QString &newPasswordDerived,
                                const QString &wrappedSyncKey,
                                const QString &syncWrapSalt,
                                const QJsonObject &syncWrapParams) const;

private:
  template<typename T, typename Parser>
  ApiResult<T> parseTypedResponse(const NetworkResponse &response, Parser parser) const {
    ApiResult<T> result;
    result.statusCode = response.statusCode;

    const auto object = JsonCodec::parseObject(response.body, &result.error);
    if (!object.has_value()) {
      if (!response.errorMessage.isEmpty()) {
        result.error = response.errorMessage;
      }
      return result;
    }

    if (response.ok()) {
      result.data = parser(*object);
      result.ok = result.data.has_value();
      if (!result.ok && result.error.isEmpty()) {
        result.error = "Unexpected response shape";
      }
      return result;
    }

    const auto apiError = parseApiError(*object);
    result.error = apiError.has_value() ? apiError->message : response.errorMessage;
    return result;
  }

  VoidResult parseVoidResponse(const NetworkResponse &response) const;

  NetworkAccess &m_network;
};

} // namespace vox::network

#endif // VOX_NETWORK_AUTHAPI_HPP
