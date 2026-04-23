#ifndef VOX_CRYPTO_IDENTITYKEYSTORE_HPP
#define VOX_CRYPTO_IDENTITYKEYSTORE_HPP

#include <QByteArray>
#include <optional>

#include "crypto/VaultService.hpp"

namespace vox::crypto {

struct IdentityKeys {
  QByteArray signingPublic;
  QByteArray signingPrivate;
  QByteArray dhPublic;
  QByteArray dhPrivate;
};

class IdentityKeyStore final {
public:
  explicit IdentityKeyStore(VaultService &vault);

  bool ensureIdentityKeys();
  std::optional<IdentityKeys> identityKeys() const;

private:
  VaultService &m_vault;
};

} // namespace vox::crypto

#endif // VOX_CRYPTO_IDENTITYKEYSTORE_HPP
