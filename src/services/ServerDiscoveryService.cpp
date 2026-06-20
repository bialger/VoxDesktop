#include "services/ServerDiscoveryService.hpp"

#include "network/ApiPaths.hpp"
#include "network/JsonCodec.hpp"

namespace vox::services {

ServerDiscoveryService::ServerDiscoveryService(network::NetworkAccess &network) : m_network(network) {
}

bool ServerDiscoveryService::checkHealth() const {
  const auto response = m_network.get(network::ApiPaths::kHealth);
  if (!response.ok()) {
    return false;
  }

  const auto object = network::JsonCodec::parseObject(response.body);
  return object.has_value() && object->value("status").toString() == "ok";
}

bool ServerDiscoveryService::validateProfile(const domain::ServerProfile &profile) const {
  return !profile.baseUrl.isEmpty() && !profile.serverId.isEmpty() && checkHealth();
}

} // namespace vox::services
