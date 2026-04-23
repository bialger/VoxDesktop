#include <gtest/gtest.h>

#include "crypto/HkdfSha256.hpp"
#include "tests/test_suites/ProjectVectorTestSuite.hpp"

TEST_F(ProjectVectorTestSuite, HkdfSha256Rfc5869Case1) {
  const QByteArray ikm(22, static_cast<char>(0x0b));
  const QByteArray salt = QByteArray::fromHex("000102030405060708090a0b0c");
  const QByteArray info = QByteArray::fromHex("f0f1f2f3f4f5f6f7f8f9");

  const QByteArray okm = vox::crypto::hkdfSha256(ikm, salt, info, 42);
  const QByteArray expected = QByteArray::fromHex(
      "3cb25f25faacd57a90434f64d0362f2a"
      "2d2d0a90cf1a5a4c5db02d56ecc4c5bf"
      "34007208d5b887185865");

  ASSERT_EQ(okm.size(), expected.size());
  EXPECT_EQ(okm.toHex(), expected.toHex());
}
