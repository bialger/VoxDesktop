#include <gtest/gtest.h>

#include "network/AttachmentsApi.hpp"
#include "network/AuthApi.hpp"
#include "network/ConversationsApi.hpp"
#include "network/NetworkAccess.hpp"
#include "network/SyncApi.hpp"
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

} // namespace

TEST_F(ProjectIntegrationTestSuite, RegisterLoginSendAndPullPendingPositive) {
    FakeVoxServer server;
    auto network = buildNetwork(server);

    vox::network::AuthApi auth(network);
    vox::network::ConversationsApi conversations(network);

    vox::network::RegisterRequest registerRequest;
    registerRequest.username = "alice";
    registerRequest.passwordDerivedValue = "pwd";
    registerRequest.deviceId = "dev_desktop";
    registerRequest.deviceLabel = "Desktop";
    registerRequest.identityKeyPublic = "idpk";
    registerRequest.signedPrekeyPublic = "spk";
    registerRequest.signedPrekeySignature = "sig";
    registerRequest.wrappedSyncKey = "wrapped";
    registerRequest.syncWrapSalt = "salt";
    registerRequest.syncWrapParams = QJsonObject{{"algorithm", "argon2id"}};

    const auto registered = auth.registerUser(registerRequest);
    ASSERT_TRUE(registered.ok);
    ASSERT_TRUE(registered.data.has_value());

    network.setBearerToken(registered.data->accessToken);

    const auto listed = conversations.listConversations();
    ASSERT_TRUE(listed.ok);
    ASSERT_TRUE(listed.data.has_value());
    ASSERT_FALSE(listed.data->isEmpty());

    vox::network::SendEnvelopeRequest sendRequest;
    sendRequest.deviceId = "dev_desktop";
    sendRequest.conversationId = "conv_1";
    sendRequest.ciphertext = "ciphertext_payload";
    sendRequest.envelopeId = "env_client_1";
    sendRequest.envelopeType = 0;

    const auto sent = conversations.sendEnvelope(sendRequest);
    ASSERT_TRUE(sent.ok);
    ASSERT_TRUE(sent.data.has_value());

    const auto pending = conversations.pendingEnvelopes(100);
    ASSERT_TRUE(pending.ok);
    ASSERT_TRUE(pending.data.has_value());
    ASSERT_FALSE(pending.data->envelopes.isEmpty());
    EXPECT_EQ(pending.data->envelopes.first().envelopeId, "env_client_1");
}

TEST_F(ProjectIntegrationTestSuite, AttachmentUploadDownloadPositive) {
    FakeVoxServer server;
    auto network = buildNetwork(server);

    vox::network::AuthApi auth(network);
    vox::network::AttachmentsApi attachments(network);

    vox::network::RegisterRequest registerRequest;
    registerRequest.username = "alice";
    registerRequest.passwordDerivedValue = "pwd";
    registerRequest.deviceId = "dev_desktop";
    registerRequest.deviceLabel = "Desktop";
    registerRequest.identityKeyPublic = "idpk";
    registerRequest.signedPrekeyPublic = "spk";
    registerRequest.signedPrekeySignature = "sig";
    registerRequest.wrappedSyncKey = "wrapped";
    registerRequest.syncWrapSalt = "salt";
    registerRequest.syncWrapParams = QJsonObject{{"algorithm", "argon2id"}};

    const auto registered = auth.registerUser(registerRequest);
    ASSERT_TRUE(registered.ok);
    network.setBearerToken(registered.data->accessToken);

    vox::network::AttachmentUploadInitRequest initReq{"conv_1", 8, "application/octet-stream"};
    const auto init = attachments.uploadInit(initReq);
    ASSERT_TRUE(init.ok);
    ASSERT_TRUE(init.data.has_value());

    const QByteArray payload("encrypted");
    EXPECT_TRUE(attachments.uploadChunk(init.data->attachmentId, 0, payload).ok);
    EXPECT_TRUE(attachments.finalize(init.data->attachmentId, "sha256:abc").ok);

    const auto downloaded = attachments.download(init.data->attachmentId);
    ASSERT_TRUE(downloaded.ok);
    ASSERT_TRUE(downloaded.data.has_value());
    EXPECT_EQ(*downloaded.data, payload);
}

TEST_F(ProjectIntegrationTestSuite, SyncChangesPullPositive) {
    FakeVoxServer server;
    auto network = buildNetwork(server);

    vox::network::AuthApi auth(network);
    vox::network::SyncApi syncApi(network);

    vox::network::RegisterRequest registerRequest;
    registerRequest.username = "alice";
    registerRequest.passwordDerivedValue = "pwd";
    registerRequest.deviceId = "dev_desktop";
    registerRequest.deviceLabel = "Desktop";
    registerRequest.identityKeyPublic = "idpk";
    registerRequest.signedPrekeyPublic = "spk";
    registerRequest.signedPrekeySignature = "sig";
    registerRequest.wrappedSyncKey = "wrapped";
    registerRequest.syncWrapSalt = "salt";
    registerRequest.syncWrapParams = QJsonObject{{"algorithm", "argon2id"}};

    const auto registered = auth.registerUser(registerRequest);
    ASSERT_TRUE(registered.ok);
    network.setBearerToken(registered.data->accessToken);

    const auto changes = syncApi.changes("contacts", {}, 100);
    ASSERT_TRUE(changes.ok);
    ASSERT_TRUE(changes.data.has_value());
    EXPECT_TRUE(changes.data->records.isEmpty());
}
