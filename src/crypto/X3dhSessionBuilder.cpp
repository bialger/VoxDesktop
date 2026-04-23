#include "crypto/X3dhSessionBuilder.hpp"

#include "crypto/CryptoHelpers.hpp"
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
    ikm.reserve(dh1.size() + dh2.size() + dh3.size() + 32);
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

    const QByteArray rootKey = hkdfSha256(ikm, "vox-x3dh-salt", "vox-e2ee-v1/x3dh", 32);
    if (rootKey.size() != 32) {
        return std::nullopt;
    }

    return rootKey;
}

} // namespace vox::crypto
