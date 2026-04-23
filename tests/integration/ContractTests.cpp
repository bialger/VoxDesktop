#include <gtest/gtest.h>

#include "network/ApiPaths.hpp"
#include "network/NetworkAccess.hpp"

TEST(ContractTests, HealthEndpointContract) {
    const auto baseUrl = qEnvironmentVariable("VOX_CONTRACT_BASE_URL");
    if (baseUrl.isEmpty()) {
        GTEST_SKIP() << "VOX_CONTRACT_BASE_URL is not configured";
    }

    vox::network::NetworkAccess network(baseUrl);
    const auto response = network.get(vox::network::ApiPaths::kHealth);

    ASSERT_TRUE(response.ok()) << response.errorMessage.toStdString();
    EXPECT_EQ(response.statusCode, 200);
}
