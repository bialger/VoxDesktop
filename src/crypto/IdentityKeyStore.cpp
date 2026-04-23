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

IdentityKeyStore::IdentityKeyStore(VaultService &vault) : m_vault(vault) {}

bool IdentityKeyStore::ensureIdentityKeys() {
    if (!m_vault.isUnlocked() || !ensureSodiumInitialized()) {
        return false;
    }

    const auto existing = identityKeys();
    if (existing.has_value()) {
        return true;
    }

    QByteArray signPk(crypto_sign_PUBLICKEYBYTES, Qt::Uninitialized);
    QByteArray signSk(crypto_sign_SECRETKEYBYTES, Qt::Uninitialized);
    if (crypto_sign_keypair(reinterpret_cast<unsigned char *>(signPk.data()),
                            reinterpret_cast<unsigned char *>(signSk.data())) != 0) {
        return false;
    }

    QByteArray dhPk(crypto_kx_PUBLICKEYBYTES, Qt::Uninitialized);
    QByteArray dhSk(crypto_kx_SECRETKEYBYTES, Qt::Uninitialized);
    if (crypto_kx_keypair(reinterpret_cast<unsigned char *>(dhPk.data()),
                          reinterpret_cast<unsigned char *>(dhSk.data())) != 0) {
        return false;
    }

    return m_vault.storeSecret(kSignPub, signPk) && m_vault.storeSecret(kSignPriv, signSk) &&
           m_vault.storeSecret(kDhPub, dhPk) && m_vault.storeSecret(kDhPriv, dhSk);
}

std::optional<IdentityKeys> IdentityKeyStore::identityKeys() const {
    const auto signPub = m_vault.loadSecret(kSignPub);
    const auto signPriv = m_vault.loadSecret(kSignPriv);
    const auto dhPub = m_vault.loadSecret(kDhPub);
    const auto dhPriv = m_vault.loadSecret(kDhPriv);
    if (!signPub.has_value() || !signPriv.has_value() || !dhPub.has_value() || !dhPriv.has_value()) {
        return std::nullopt;
    }

    return IdentityKeys{*signPub, *signPriv, *dhPub, *dhPriv};
}

} // namespace vox::crypto
