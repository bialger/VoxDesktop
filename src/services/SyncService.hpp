#ifndef VOX_SERVICES_SYNCSERVICE_HPP
#define VOX_SERVICES_SYNCSERVICE_HPP

#include "network/SyncApi.hpp"
#include "services/Interfaces.hpp"
#include "storage/Repositories.hpp"

namespace vox::services {

class SyncService final : public ISyncService {
public:
  SyncService(network::SyncApi &api, storage::ISyncRepository &repository);

  bool pullCollection(const QString &collection) override;
  bool pushRecord(const domain::SyncRecord &record) override;

private:
  network::SyncApi &m_api;
  storage::ISyncRepository &m_repository;
};

} // namespace vox::services

#endif // VOX_SERVICES_SYNCSERVICE_HPP
