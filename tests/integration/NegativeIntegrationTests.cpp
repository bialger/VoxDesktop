#include <gtest/gtest.h>

#include "network/AuthApi.hpp"
#include "network/ConversationsApi.hpp"
#include "network/NetworkAccess.hpp"
#include "tests/integration/FakeVoxServer.hpp"
#include "tests/test_suites/ProjectIntegrationTestSuite.hpp"

namespace {

vox::network::NetworkAccess BuildNetwork(FakeVoxServer &server) {
  vox::network::NetworkAccess network("http://fake");
  network.setCustomHandler([&](const QString &method,
                               const QString &path,
                               const QByteArray &body,
                               const vox::network::NetworkAccess::HeaderMap &headers) {
    return server.Handle(method, path, body, headers);
  });
  return network;
}

vox::network::RegisterRequest MinimalRegisterRequest() {
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
  auto network = BuildNetwork(server);

  vox::network::ConversationsApi conversations(network);
  const auto result = conversations.listConversations();

  EXPECT_FALSE(result.ok);
  EXPECT_EQ(result.statusCode, 401);
}

TEST_F(ProjectIntegrationTestSuite, RefreshFailsWithInvalidTokenNegative) {
  FakeVoxServer server;
  auto network = BuildNetwork(server);

  vox::network::AuthApi auth(network);

  const auto result =
      auth.refresh(vox::network::RefreshRequest{.refreshToken = "invalid_refresh", .deviceId = "dev_desktop"});
  EXPECT_FALSE(result.ok);
  EXPECT_EQ(result.statusCode, 401);
}

TEST_F(ProjectIntegrationTestSuite, SendFailsWhenServerQueueIsFullNegative) {
  FakeVoxServer server;
  auto network = BuildNetwork(server);

  vox::network::AuthApi auth(network);
  vox::network::ConversationsApi conversations(network);

  const auto registered = auth.registerUser(MinimalRegisterRequest());
  ASSERT_TRUE(registered.ok);
  ASSERT_TRUE(registered.data.has_value());
  if (!registered.data.has_value()) {
    return;
  }

  network.setBearerToken(registered.data->accessToken);

  server.SetFailNextSend(true);

  vox::network::SendEnvelopeRequest send_request;
  send_request.deviceId = "dev_desktop";
  send_request.conversationId = "conv_1";
  send_request.ciphertext = "ciphertext_payload";
  send_request.envelopeId = "env_client_1";
  send_request.envelopeType = 0;

  const auto sent = conversations.sendEnvelope(send_request);
  EXPECT_FALSE(sent.ok);
  EXPECT_EQ(sent.statusCode, 503);
}

TEST_F(ProjectIntegrationTestSuite, UnauthorizedAfterServerSideInvalidationNegative) {
  FakeVoxServer server;
  auto network = BuildNetwork(server);

  vox::network::AuthApi auth(network);
  vox::network::ConversationsApi conversations(network);

  const auto registered = auth.registerUser(MinimalRegisterRequest());
  ASSERT_TRUE(registered.ok);
  ASSERT_TRUE(registered.data.has_value());
  if (!registered.data.has_value()) {
    return;
  }

  network.setBearerToken(registered.data->accessToken);
  server.SetForceUnauthorized(true);

  const auto pending = conversations.pendingEnvelopes(100);
  EXPECT_FALSE(pending.ok);
  EXPECT_EQ(pending.statusCode, 401);
}
