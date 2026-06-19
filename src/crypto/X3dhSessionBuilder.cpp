#include "crypto/X3dhSessionBuilder.hpp"

#include "crypto/CryptoHelpers.hpp"
#include "crypto/CryptoSizes.hpp"
#include "crypto/HkdfSha256.hpp"

namespace vox::crypto {

std::optional<QByteArray> X3dhSessionBuilder::buildInitialSharedSecret(const X3dhInput &input) {
  const QByteArray dh1 = CryptoHelpers::x25519(input.ourIdentityPrivate, input.theirSignedPrekeyPublic);
  const QByteArray dh2 = CryptoHelpers::x25519(input.ourEphemeralPrivate, input.theirIdentityPublic);
  const QByteArray dh3 = CryptoHelpers::x25519(input.ourEphemeralPrivate, input.theirSignedPrekeyPublic);

  if (dh1.isEmpty() || dh2.isEmpty() || dh3.isEmpty()) {
    return std::nullopt;
  }

  QByteArray ikm;
  ikm.reserve(dh1.size() + dh2.size() + dh3.size() + kChaCha20KeyBytes);
  ikm.append(dh1);
  ikm.append(dh2);
  ikm.append(dh3);

  if (input.theirOneTimePrekeyPublic.has_value()) {
    const QByteArray dh4 = CryptoHelpers::x25519(input.ourEphemeralPrivate, *input.theirOneTimePrekeyPublic);
    if (dh4.isEmpty()) {
      return std::nullopt;
    }
    ikm.append(dh4);
  }

  QByteArray root_key = hkdfSha256(ikm, "vox-x3dh-salt", "vox-e2ee-v1/x3dh", kChaCha20KeyBytes);
  if (root_key.size() != kChaCha20KeyBytes) {
    return std::nullopt;
  }

  return root_key;
}

} // namespace vox::crypto
