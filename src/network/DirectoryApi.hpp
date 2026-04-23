#ifndef VOX_NETWORK_DIRECTORYAPI_HPP
#define VOX_NETWORK_DIRECTORYAPI_HPP

#include "network/ApiDtos.hpp"
#include "network/ApiResult.hpp"
#include "network/NetworkAccess.hpp"

namespace vox::network {

class DirectoryApi final {
public:
  explicit DirectoryApi(NetworkAccess &network);

  ApiResult<DirectoryUser> userByUsername(const QString &username) const;
  ApiResult<DirectoryUser> userById(const QString &userId) const;
  ApiResult<QVector<DirectoryUser>> searchUsers(const QString &query, int limit) const;
  ApiResult<QVector<DeviceDirectoryEntry>> userDevices(const QString &userId) const;

private:
  NetworkAccess &m_network;
};

} // namespace vox::network

#endif // VOX_NETWORK_DIRECTORYAPI_HPP
