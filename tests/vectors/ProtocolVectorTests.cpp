#include <gtest/gtest.h>

#include "crypto/DoubleRatchetSession.hpp"
#include "crypto/SyncCrypto.hpp"
#include "tests/test_suites/ProjectVectorTestSuite.hpp"

TEST_F(ProjectVectorTestSuite, DoubleRatchetRoundTripVector) {
  const QByteArray sharedSecret =
      QByteArray::fromHex("00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff");

  auto sender = vox::crypto::DoubleRatchetSession::fromSharedSecret(sharedSecret, "ad");
  auto receiver = vox::crypto::DoubleRatchetSession::fromSharedSecret(sharedSecret, "ad");

  const auto encrypted = sender.encrypt("hello-vector", "meta");
  ASSERT_TRUE(encrypted.has_value());

  const auto decrypted = receiver.decrypt(*encrypted, "meta");
  ASSERT_TRUE(decrypted.has_value());
  EXPECT_EQ(*decrypted, "hello-vector");
}

TEST_F(ProjectVectorTestSuite, SyncWrapUnwrapVector) {
  const QByteArray passwordDerived = "derived-password-value";
  const QByteArray syncMasterKey =
      QByteArray::fromHex("11223344556677889900aabbccddeeff00112233445566778899aabbccddeeff");

  const auto wrapped = vox::crypto::SyncCrypto::wrapSyncMasterKey(passwordDerived, syncMasterKey);
  ASSERT_TRUE(wrapped.has_value());

  const auto unwrapped = vox::crypto::SyncCrypto::unwrapSyncMasterKey(passwordDerived, *wrapped);
  ASSERT_TRUE(unwrapped.has_value());
  EXPECT_EQ(*unwrapped, syncMasterKey);
}
