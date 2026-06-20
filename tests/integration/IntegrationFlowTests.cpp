#include <gtest/gtest.h>

#include "network/AttachmentsApi.hpp"
#include "network/AuthApi.hpp"
#include "network/ConversationsApi.hpp"
#include "network/NetworkAccess.hpp"
#include "network/SyncApi.hpp"
#include "tests/integration/FakeVoxServer.hpp"
#include "tests/test_suites/ProjectIntegrationTestSuite.hpp"

namespace {

constexpr int kAttachmentPayloadBytes = 8;

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

} // namespace

TEST_F(ProjectIntegrationTestSuite, RegisterLoginSendAndPullPendingPositive) {
  FakeVoxServer server;
  auto network = BuildNetwork(server);

  vox::network::AuthApi auth(network);
  vox::network::ConversationsApi conversations(network);

  vox::network::RegisterRequest register_request;
  register_request.username = "alice";
  register_request.passwordDerivedValue = "pwd";
  register_request.deviceId = "dev_desktop";
  register_request.deviceLabel = "Desktop";
  register_request.identityKeyPublic = "idpk";
  register_request.signedPrekeyPublic = "spk";
  register_request.signedPrekeySignature = "sig";
  register_request.wrappedSyncKey = "wrapped";
  register_request.syncWrapSalt = "salt";
  register_request.syncWrapParams = QJsonObject{{"algorithm", "argon2id"}};

  const auto registered = auth.registerUser(register_request);
  ASSERT_TRUE(registered.ok);
  ASSERT_TRUE(registered.data.has_value());
  if (!registered.data.has_value()) {
    return;
  }
  network.setBearerToken(registered.data->accessToken);

  const auto listed = conversations.listConversations();
  ASSERT_TRUE(listed.ok);
  ASSERT_TRUE(listed.data.has_value());
  if (!listed.data.has_value()) {
    return;
  }
  ASSERT_FALSE(listed.data->isEmpty());

  vox::network::SendEnvelopeRequest send_request;
  send_request.deviceId = "dev_desktop";
  send_request.conversationId = "conv_1";
  send_request.ciphertext = "ciphertext_payload";
  send_request.envelopeId = "env_client_1";
  send_request.envelopeType = 0;

  const auto sent = conversations.sendEnvelope(send_request);
  ASSERT_TRUE(sent.ok);
  ASSERT_TRUE(sent.data.has_value());

  const auto pending = conversations.pendingEnvelopes(100);
  ASSERT_TRUE(pending.ok);
  ASSERT_TRUE(pending.data.has_value());
  if (!pending.data.has_value()) {
    return;
  }
  ASSERT_FALSE(pending.data->envelopes.isEmpty());
  EXPECT_EQ(pending.data->envelopes.first().envelopeId, "env_client_1");
}

TEST_F(ProjectIntegrationTestSuite, AttachmentUploadDownloadPositive) {
  FakeVoxServer server;
  auto network = BuildNetwork(server);

  vox::network::AuthApi auth(network);
  vox::network::AttachmentsApi attachments(network);

  vox::network::RegisterRequest register_request;
  register_request.username = "alice";
  register_request.passwordDerivedValue = "pwd";
  register_request.deviceId = "dev_desktop";
  register_request.deviceLabel = "Desktop";
  register_request.identityKeyPublic = "idpk";
  register_request.signedPrekeyPublic = "spk";
  register_request.signedPrekeySignature = "sig";
  register_request.wrappedSyncKey = "wrapped";
  register_request.syncWrapSalt = "salt";
  register_request.syncWrapParams = QJsonObject{{"algorithm", "argon2id"}};

  const auto registered = auth.registerUser(register_request);
  ASSERT_TRUE(registered.ok);
  ASSERT_TRUE(registered.data.has_value());
  if (!registered.data.has_value()) {
    return;
  }
  network.setBearerToken(registered.data->accessToken);

  vox::network::AttachmentUploadInitRequest init_req{
      .conversationId = "conv_1", .fileSize = kAttachmentPayloadBytes, .mimeHint = "application/octet-stream"};
  const auto init = attachments.uploadInit(init_req);
  ASSERT_TRUE(init.ok);
  ASSERT_TRUE(init.data.has_value());
  if (!init.data.has_value()) {
    return;
  }

  const QByteArray payload("encrypted");
  EXPECT_TRUE(attachments.uploadChunk(init.data->attachmentId, 0, payload).ok);
  EXPECT_TRUE(attachments.finalize(init.data->attachmentId, "sha256:abc").ok);

  const auto downloaded = attachments.download(init.data->attachmentId);
  ASSERT_TRUE(downloaded.ok);
  ASSERT_TRUE(downloaded.data.has_value());
  if (!downloaded.data.has_value()) {
    return;
  }
  EXPECT_EQ(*downloaded.data, payload);
}

TEST_F(ProjectIntegrationTestSuite, SyncChangesPullPositive) {
  FakeVoxServer server;
  auto network = BuildNetwork(server);

  vox::network::AuthApi auth(network);
  vox::network::SyncApi sync_api(network);

  vox::network::RegisterRequest register_request;
  register_request.username = "alice";
  register_request.passwordDerivedValue = "pwd";
  register_request.deviceId = "dev_desktop";
  register_request.deviceLabel = "Desktop";
  register_request.identityKeyPublic = "idpk";
  register_request.signedPrekeyPublic = "spk";
  register_request.signedPrekeySignature = "sig";
  register_request.wrappedSyncKey = "wrapped";
  register_request.syncWrapSalt = "salt";
  register_request.syncWrapParams = QJsonObject{{"algorithm", "argon2id"}};

  const auto registered = auth.registerUser(register_request);
  ASSERT_TRUE(registered.ok);
  ASSERT_TRUE(registered.data.has_value());
  if (!registered.data.has_value()) {
    return;
  }
  network.setBearerToken(registered.data->accessToken);

  const auto changes = sync_api.changes("contacts", {}, 100);
  ASSERT_TRUE(changes.ok);
  ASSERT_TRUE(changes.data.has_value());
  if (!changes.data.has_value()) {
    return;
  }
  EXPECT_TRUE(changes.data->records.isEmpty());
}
