#ifndef VOX_CRYPTO_X3DHSESSIONBUILDER_HPP
#define VOX_CRYPTO_X3DHSESSIONBUILDER_HPP

#include <QByteArray>
#include <optional>

namespace vox::crypto {

struct X3dhInput {
    QByteArray ourIdentityPrivate;
    QByteArray ourEphemeralPrivate;
    QByteArray theirIdentityPublic;
    QByteArray theirSignedPrekeyPublic;
    std::optional<QByteArray> theirOneTimePrekeyPublic;
};

class X3dhSessionBuilder final {
public:
    static std::optional<QByteArray> buildInitialSharedSecret(const X3dhInput &input);
};

} // namespace vox::crypto

#endif // VOX_CRYPTO_X3DHSESSIONBUILDER_HPP
