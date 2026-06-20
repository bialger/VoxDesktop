#include <gtest/gtest.h>

#include "network/ApiPaths.hpp"
#include "network/NetworkAccess.hpp"

TEST(ContractTests, HealthEndpointContract) {
  const auto base_url = qEnvironmentVariable("VOX_CONTRACT_BASE_URL");
  if (base_url.isEmpty()) {
    GTEST_SKIP() << "VOX_CONTRACT_BASE_URL is not configured";
  }

  vox::network::NetworkAccess network(base_url);
  const auto response = network.get(vox::network::ApiPaths::kHealth);

  ASSERT_TRUE(response.ok()) << response.errorMessage.toStdString();
  EXPECT_EQ(response.statusCode, 200);
}
