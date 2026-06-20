#ifndef VOX_SERVICES_SERVERDISCOVERYSERVICE_HPP
#define VOX_SERVICES_SERVERDISCOVERYSERVICE_HPP

#include "domain/Models.hpp"
#include "network/NetworkAccess.hpp"

namespace vox::services {

class ServerDiscoveryService final {
public:
  explicit ServerDiscoveryService(network::NetworkAccess &network);

  bool checkHealth() const;
  bool validateProfile(const domain::ServerProfile &profile) const;

private:
  network::NetworkAccess &m_network;
};

} // namespace vox::services

#endif // VOX_SERVICES_SERVERDISCOVERYSERVICE_HPP
