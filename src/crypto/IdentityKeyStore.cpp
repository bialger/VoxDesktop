#include "crypto/IdentityKeyStore.hpp"

#include "crypto/SodiumInit.hpp"

#include <sodium.h>

namespace vox::crypto {
namespace {
constexpr auto kSignPub = "identity_sign_pub";
constexpr auto kSignPriv = "identity_sign_priv";
constexpr auto kDhPub = "identity_dh_pub";
constexpr auto kDhPriv = "identity_dh_priv";
} // namespace

IdentityKeyStore::IdentityKeyStore(VaultService &vault) : m_vault(vault) {
}

bool IdentityKeyStore::ensureIdentityKeys() {
  if (!m_vault.isUnlocked() || !ensureSodiumInitialized()) {
    return false;
  }

  const auto existing = identityKeys();
  if (existing.has_value()) {
    return true;
  }

  QByteArray sign_pk(crypto_sign_PUBLICKEYBYTES, Qt::Uninitialized);
  QByteArray sign_sk(crypto_sign_SECRETKEYBYTES, Qt::Uninitialized);
  if (crypto_sign_keypair(reinterpret_cast<unsigned char *>(sign_pk.data()),
                          reinterpret_cast<unsigned char *>(sign_sk.data())) != 0) {
    return false;
  }

  QByteArray dh_pk(crypto_kx_PUBLICKEYBYTES, Qt::Uninitialized);
  QByteArray dh_sk(crypto_kx_SECRETKEYBYTES, Qt::Uninitialized);
  if (crypto_kx_keypair(reinterpret_cast<unsigned char *>(dh_pk.data()),
                        reinterpret_cast<unsigned char *>(dh_sk.data())) != 0) {
    return false;
  }

  return m_vault.storeSecret(kSignPub, sign_pk) && m_vault.storeSecret(kSignPriv, sign_sk) &&
         m_vault.storeSecret(kDhPub, dh_pk) && m_vault.storeSecret(kDhPriv, dh_sk);
}

std::optional<IdentityKeys> IdentityKeyStore::identityKeys() const {
  const auto sign_pub = m_vault.loadSecret(kSignPub);
  const auto sign_priv = m_vault.loadSecret(kSignPriv);
  const auto dh_pub = m_vault.loadSecret(kDhPub);
  const auto dh_priv = m_vault.loadSecret(kDhPriv);
  if (!sign_pub.has_value() || !sign_priv.has_value() || !dh_pub.has_value() || !dh_priv.has_value()) {
    return std::nullopt;
  }

  return IdentityKeys{
      .signingPublic = *sign_pub, .signingPrivate = *sign_priv, .dhPublic = *dh_pub, .dhPrivate = *dh_priv};
}

} // namespace vox::crypto
