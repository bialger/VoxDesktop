#include <gtest/gtest.h>

#include "network/ApiDtos.hpp"
#include "network/JsonCodec.hpp"
#include "tests/test_suites/ProjectUnitTestSuite.hpp"

TEST_F(ProjectUnitTestSuite, JsonCodecParsesValidObject) {
    const QByteArray payload = R"({"user_id":"usr_1","access_token":"acc","refresh_token":"ref","device_status":"created","sync_key_version":1})";
    const auto parsed = vox::network::JsonCodec::parseObject(payload);
    ASSERT_TRUE(parsed.has_value());

    const auto auth = vox::network::parseAuthSessionResponse(*parsed);
    ASSERT_TRUE(auth.has_value());
    EXPECT_EQ(auth->userId, "usr_1");
    EXPECT_EQ(auth->deviceStatus, "created");
}

TEST_F(ProjectUnitTestSuite, JsonCodecRejectsMalformedJson) {
    const QByteArray payload = R"({"broken":)";
    const auto parsed = vox::network::JsonCodec::parseObject(payload);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(ProjectUnitTestSuite, EnvelopeDtoParsesCanonicalFields) {
    const QByteArray payload =
        R"({"envelope_id":"env_1","conversation_id":"conv_1","sender_user_id":"usr_bob","sender_device_id":"dev_remote","ciphertext":"abc","server_timestamp":1710004000,"envelope_type":0,"ordering_epoch":3})";

    const auto parsed = vox::network::JsonCodec::parseObject(payload);
    ASSERT_TRUE(parsed.has_value());

    const auto envelope = vox::network::parseEnvelopeDto(*parsed);
    ASSERT_TRUE(envelope.has_value());
    EXPECT_EQ(envelope->envelopeId, "env_1");
    ASSERT_TRUE(envelope->orderingEpoch.has_value());
    EXPECT_EQ(*envelope->orderingEpoch, 3);
}

TEST_F(ProjectUnitTestSuite, EnvelopeDtoFailsWhenRequiredFieldMissing) {
    const QByteArray payload =
        R"({"conversation_id":"conv_1","sender_user_id":"usr_bob","sender_device_id":"dev_remote","ciphertext":"abc","server_timestamp":1710004000})";

    const auto parsed = vox::network::JsonCodec::parseObject(payload);
    ASSERT_TRUE(parsed.has_value());

    const auto envelope = vox::network::parseEnvelopeDto(*parsed);
    EXPECT_FALSE(envelope.has_value());
}
