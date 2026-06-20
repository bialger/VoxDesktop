#ifndef VOX_NETWORK_SYNCAPI_HPP
#define VOX_NETWORK_SYNCAPI_HPP

#include "network/ApiDtos.hpp"
#include "network/ApiResult.hpp"
#include "network/NetworkAccess.hpp"

namespace vox::network {

struct SyncChangesResponse {
  QVector<SyncRecordDto> records;
  QString nextCursor;
  bool hasMore{false};
};

class SyncApi final {
public:
  explicit SyncApi(NetworkAccess &network);

  ApiResult<SyncKeyBundleDto> getKeyBundle() const;
  VoidResult putKeyBundle(const SyncKeyBundleDto &bundle) const;

  ApiResult<SyncChangesResponse> changes(const QString &collection, const QString &cursor, int limit) const;

  VoidResult putRecord(const SyncRecordDto &record) const;
  VoidResult deleteRecord(const QString &collection, const QString &recordId) const;

private:
  NetworkAccess &m_network;
};

} // namespace vox::network

#endif // VOX_NETWORK_SYNCAPI_HPP
