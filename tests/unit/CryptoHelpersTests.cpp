#include <sodium.h>

#include <gtest/gtest.h>

#include "crypto/CryptoHelpers.hpp"
#include "crypto/HkdfSha256.hpp"
#include "tests/test_suites/ProjectUnitTestSuite.hpp"

TEST_F(ProjectUnitTestSuite, HkdfIsDeterministicForSameInput) {
    const QByteArray ikm("input-key-material");
    const QByteArray salt("salt-value");
    const QByteArray info("vox-e2ee-v1/test");

    const QByteArray first = vox::crypto::hkdfSha256(ikm, salt, info, 64);
    const QByteArray second = vox::crypto::hkdfSha256(ikm, salt, info, 64);

    ASSERT_EQ(first, second);
    ASSERT_EQ(first.size(), 64);
}

TEST_F(ProjectUnitTestSuite, AeadRoundTripWorks) {
    const QByteArray key = vox::crypto::CryptoHelpers::randomBytes(32);
    const QByteArray nonce = vox::crypto::CryptoHelpers::randomBytes(12);
    const QByteArray ad("vox-ad");
    const QByteArray plaintext("hello encrypted world");

    const QByteArray ciphertext = vox::crypto::CryptoHelpers::aeadEncrypt(key, nonce, plaintext, ad);
    ASSERT_FALSE(ciphertext.isEmpty());

    const auto decrypted = vox::crypto::CryptoHelpers::aeadDecrypt(key, nonce, ciphertext, ad);
    ASSERT_TRUE(decrypted.has_value());
    EXPECT_EQ(*decrypted, plaintext);
}

TEST_F(ProjectUnitTestSuite, AeadDecryptFailsWhenCiphertextTampered) {
    const QByteArray key = vox::crypto::CryptoHelpers::randomBytes(32);
    const QByteArray nonce = vox::crypto::CryptoHelpers::randomBytes(12);

    QByteArray ciphertext = vox::crypto::CryptoHelpers::aeadEncrypt(key, nonce, "payload", "ad");
    ASSERT_FALSE(ciphertext.isEmpty());
    ciphertext[0] = static_cast<char>(ciphertext[0] ^ 0x01);

    const auto decrypted = vox::crypto::CryptoHelpers::aeadDecrypt(key, nonce, ciphertext, "ad");
    EXPECT_FALSE(decrypted.has_value());
}

TEST_F(ProjectUnitTestSuite, AeadDecryptFailsOnInvalidNonceLength) {
    const QByteArray key = vox::crypto::CryptoHelpers::randomBytes(32);
    const auto decrypted = vox::crypto::CryptoHelpers::aeadDecrypt(key, QByteArray(11, '\0'), "ciphertext", "ad");
    EXPECT_FALSE(decrypted.has_value());
}

TEST_F(ProjectUnitTestSuite, X25519ProducesSameSharedSecretBothDirections) {
    QByteArray aPk(crypto_kx_PUBLICKEYBYTES, Qt::Uninitialized);
    QByteArray aSk(crypto_kx_SECRETKEYBYTES, Qt::Uninitialized);
    QByteArray bPk(crypto_kx_PUBLICKEYBYTES, Qt::Uninitialized);
    QByteArray bSk(crypto_kx_SECRETKEYBYTES, Qt::Uninitialized);

    ASSERT_EQ(crypto_kx_keypair(reinterpret_cast<unsigned char *>(aPk.data()),
                                reinterpret_cast<unsigned char *>(aSk.data())),
              0);
    ASSERT_EQ(crypto_kx_keypair(reinterpret_cast<unsigned char *>(bPk.data()),
                                reinterpret_cast<unsigned char *>(bSk.data())),
              0);

    const QByteArray ab = vox::crypto::CryptoHelpers::x25519(aSk, bPk);
    const QByteArray ba = vox::crypto::CryptoHelpers::x25519(bSk, aPk);

    ASSERT_FALSE(ab.isEmpty());
    ASSERT_FALSE(ba.isEmpty());
    EXPECT_EQ(ab, ba);
}

TEST_F(ProjectUnitTestSuite, Ed25519SignAndVerifyWorks) {
    QByteArray pub(crypto_sign_PUBLICKEYBYTES, Qt::Uninitialized);
    QByteArray priv(crypto_sign_SECRETKEYBYTES, Qt::Uninitialized);
    ASSERT_EQ(crypto_sign_keypair(reinterpret_cast<unsigned char *>(pub.data()),
                                  reinterpret_cast<unsigned char *>(priv.data())),
              0);

    const QByteArray message("vox message");
    const QByteArray signature = vox::crypto::CryptoHelpers::ed25519Sign(priv, message);

    ASSERT_FALSE(signature.isEmpty());
    EXPECT_TRUE(vox::crypto::CryptoHelpers::ed25519Verify(pub, message, signature));

    QByteArray tampered = message;
    tampered[0] = static_cast<char>(tampered[0] ^ 0x10);
    EXPECT_FALSE(vox::crypto::CryptoHelpers::ed25519Verify(pub, tampered, signature));
}
