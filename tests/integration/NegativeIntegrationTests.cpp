#include <gtest/gtest.h>

#include "network/AuthApi.hpp"
#include "network/ConversationsApi.hpp"
#include "network/NetworkAccess.hpp"
#include "tests/integration/FakeVoxServer.hpp"
#include "tests/test_suites/ProjectIntegrationTestSuite.hpp"

namespace {

vox::network::NetworkAccess buildNetwork(FakeVoxServer &server) {
  vox::network::NetworkAccess network("http://fake");
  network.setCustomHandler([&](const QString &method,
                               const QString &path,
                               const QByteArray &body,
                               const vox::network::NetworkAccess::HeaderMap &headers) {
    return server.handle(method, path, body, headers);
  });
  return network;
}

vox::network::RegisterRequest minimalRegisterRequest() {
  vox::network::RegisterRequest request;
  request.username = "alice";
  request.passwordDerivedValue = "pwd";
  request.deviceId = "dev_desktop";
  request.deviceLabel = "Desktop";
  request.identityKeyPublic = "idpk";
  request.signedPrekeyPublic = "spk";
  request.signedPrekeySignature = "sig";
  request.wrappedSyncKey = "wrapped";
  request.syncWrapSalt = "salt";
  request.syncWrapParams = QJsonObject{{"algorithm", "argon2id"}};
  return request;
}

} // namespace

TEST_F(ProjectIntegrationTestSuite, ConversationsRequireBearerTokenNegative) {
  FakeVoxServer server;
  auto network = buildNetwork(server);

  vox::network::ConversationsApi conversations(network);
  const auto result = conversations.listConversations();

  EXPECT_FALSE(result.ok);
  EXPECT_EQ(result.statusCode, 401);
}

TEST_F(ProjectIntegrationTestSuite, RefreshFailsWithInvalidTokenNegative) {
  FakeVoxServer server;
  auto network = buildNetwork(server);

  vox::network::AuthApi auth(network);

  const auto result = auth.refresh(vox::network::RefreshRequest{"invalid_refresh", "dev_desktop"});
  EXPECT_FALSE(result.ok);
  EXPECT_EQ(result.statusCode, 401);
}

TEST_F(ProjectIntegrationTestSuite, SendFailsWhenServerQueueIsFullNegative) {
  FakeVoxServer server;
  auto network = buildNetwork(server);

  vox::network::AuthApi auth(network);
  vox::network::ConversationsApi conversations(network);

  const auto registered = auth.registerUser(minimalRegisterRequest());
  ASSERT_TRUE(registered.ok);
  ASSERT_TRUE(registered.data.has_value());

  network.setBearerToken(registered.data->accessToken);

  server.setFailNextSend(true);

  vox::network::SendEnvelopeRequest sendRequest;
  sendRequest.deviceId = "dev_desktop";
  sendRequest.conversationId = "conv_1";
  sendRequest.ciphertext = "ciphertext_payload";
  sendRequest.envelopeId = "env_client_1";
  sendRequest.envelopeType = 0;

  const auto sent = conversations.sendEnvelope(sendRequest);
  EXPECT_FALSE(sent.ok);
  EXPECT_EQ(sent.statusCode, 503);
}

TEST_F(ProjectIntegrationTestSuite, UnauthorizedAfterServerSideInvalidationNegative) {
  FakeVoxServer server;
  auto network = buildNetwork(server);

  vox::network::AuthApi auth(network);
  vox::network::ConversationsApi conversations(network);

  const auto registered = auth.registerUser(minimalRegisterRequest());
  ASSERT_TRUE(registered.ok);
  ASSERT_TRUE(registered.data.has_value());

  network.setBearerToken(registered.data->accessToken);
  server.setForceUnauthorized(true);

  const auto pending = conversations.pendingEnvelopes(100);
  EXPECT_FALSE(pending.ok);
  EXPECT_EQ(pending.statusCode, 401);
}
