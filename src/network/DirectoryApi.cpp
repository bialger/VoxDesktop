#include "network/DirectoryApi.hpp"

#include "network/ApiPaths.hpp"
#include "network/JsonCodec.hpp"

#include <QJsonArray>
#include <QUrlQuery>

namespace vox::network {

DirectoryApi::DirectoryApi(NetworkAccess &network) : m_network(network) {
}

ApiResult<DirectoryUser> DirectoryApi::userByUsername(const QString &username) const {
  const auto response = m_network.get(ApiPaths::kUsersByUsernamePrefix + username);

  ApiResult<DirectoryUser> result;
  result.statusCode = response.statusCode;

  const auto object = JsonCodec::parseObject(response.body, &result.error);
  if (!object.has_value()) {
    return result;
  }

  if (response.ok()) {
    result.data = parseDirectoryUser(*object);
    result.ok = result.data.has_value();
    if (!result.ok) {
      result.error = "Invalid directory user payload";
    }
    return result;
  }

  const auto api_error = parseApiError(*object);
  result.error = api_error.has_value() ? api_error->message : response.errorMessage;
  return result;
}

ApiResult<DirectoryUser> DirectoryApi::userById(const QString &userId) const {
  const auto response = m_network.get(ApiPaths::kUsersPrefix + userId);

  ApiResult<DirectoryUser> result;
  result.statusCode = response.statusCode;

  const auto object = JsonCodec::parseObject(response.body, &result.error);
  if (!object.has_value()) {
    return result;
  }

  if (response.ok()) {
    result.data = parseDirectoryUser(*object);
    result.ok = result.data.has_value();
    if (!result.ok) {
      result.error = "Invalid directory user payload";
    }
    return result;
  }

  const auto api_error = parseApiError(*object);
  result.error = api_error.has_value() ? api_error->message : response.errorMessage;
  return result;
}

ApiResult<QVector<DirectoryUser>> DirectoryApi::searchUsers(const QString &query, int limit) const {
  QUrlQuery params;
  params.addQueryItem("q", query);
  params.addQueryItem("limit", QString::number(limit));

  const auto response = m_network.get(ApiPaths::kUsersSearch, params);

  ApiResult<QVector<DirectoryUser>> result;
  result.statusCode = response.statusCode;

  const auto object = JsonCodec::parseObject(response.body, &result.error);
  if (!object.has_value()) {
    return result;
  }

  if (response.ok()) {
    QVector<DirectoryUser> users;
    const auto users_array = object->value("users").toArray();
    users.reserve(users_array.size());

    for (const auto &item : users_array) {
      const auto user = parseDirectoryUser(item.toObject());
      if (!user.has_value()) {
        result.error = "Invalid user entry in search result";
        return result;
      }
      users.push_back(*user);
    }

    result.data = users;
    result.ok = true;
    return result;
  }

  const auto api_error = parseApiError(*object);
  result.error = api_error.has_value() ? api_error->message : response.errorMessage;
  return result;
}

ApiResult<QVector<DeviceDirectoryEntry>> DirectoryApi::userDevices(const QString &userId) const {
  const auto response = m_network.get(ApiPaths::kUsersPrefix + userId + "/devices");

  ApiResult<QVector<DeviceDirectoryEntry>> result;
  result.statusCode = response.statusCode;

  const auto object = JsonCodec::parseObject(response.body, &result.error);
  if (!object.has_value()) {
    return result;
  }

  if (response.ok()) {
    QVector<DeviceDirectoryEntry> devices;
    const auto array = object->value("devices").toArray();
    devices.reserve(array.size());

    for (const auto &item : array) {
      const auto parsed = parseDeviceDirectoryEntry(item.toObject());
      if (!parsed.has_value()) {
        result.error = "Invalid device entry";
        return result;
      }
      devices.push_back(*parsed);
    }

    result.data = devices;
    result.ok = true;
    return result;
  }

  const auto api_error = parseApiError(*object);
  result.error = api_error.has_value() ? api_error->message : response.errorMessage;
  return result;
}

} // namespace vox::network
