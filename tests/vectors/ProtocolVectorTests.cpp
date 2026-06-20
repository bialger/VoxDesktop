#include <gtest/gtest.h>

#include "crypto/DoubleRatchetSession.hpp"
#include "crypto/SyncCrypto.hpp"
#include "tests/test_suites/ProjectVectorTestSuite.hpp"

TEST_F(ProjectVectorTestSuite, DoubleRatchetRoundTripVector) {
  const QByteArray shared_secret =
      QByteArray::fromHex("00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff");

  auto sender = vox::crypto::DoubleRatchetSession::fromSharedSecret(shared_secret, "ad");
  auto receiver = vox::crypto::DoubleRatchetSession::fromSharedSecret(shared_secret, "ad");

  const auto encrypted = sender.encrypt("hello-vector", "meta");
  ASSERT_TRUE(encrypted.has_value());
  if (!encrypted.has_value()) {
    return;
  }

  const auto decrypted = receiver.decrypt(*encrypted, "meta");
  ASSERT_TRUE(decrypted.has_value());
  if (!decrypted.has_value()) {
    return;
  }
  EXPECT_EQ(*decrypted, "hello-vector");
}

TEST_F(ProjectVectorTestSuite, SyncWrapUnwrapVector) {
  const QByteArray password_derived = "derived-password-value";
  const QByteArray sync_master_key =
      QByteArray::fromHex("11223344556677889900aabbccddeeff00112233445566778899aabbccddeeff");

  const auto wrapped = vox::crypto::SyncCrypto::wrapSyncMasterKey(password_derived, sync_master_key);
  ASSERT_TRUE(wrapped.has_value());
  if (!wrapped.has_value()) {
    return;
  }

  const auto unwrapped = vox::crypto::SyncCrypto::unwrapSyncMasterKey(password_derived, *wrapped);
  ASSERT_TRUE(unwrapped.has_value());
  if (!unwrapped.has_value()) {
    return;
  }
  EXPECT_EQ(*unwrapped, sync_master_key);
}
