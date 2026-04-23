#ifndef VOX_CRYPTO_PREKEYMANAGER_HPP
#define VOX_CRYPTO_PREKEYMANAGER_HPP

#include <QByteArray>
#include <QVector>
#include <optional>

#include "crypto/IdentityKeyStore.hpp"

namespace vox::crypto {

struct SignedPrekey {
    int keyId{0};
    QByteArray publicKey;
    QByteArray privateKey;
    QByteArray signature;
};

class PrekeyManager final {
public:
    PrekeyManager(VaultService &vault, IdentityKeyStore &identityKeyStore);

    std::optional<SignedPrekey> rotateSignedPrekey();
    QVector<QByteArray> publishOneTimePrekeys(int count);
    std::optional<QByteArray> consumeOneTimePrekey();

private:
    QByteArray encodeOneTimeQueue(const QVector<QByteArray> &keys) const;
    QVector<QByteArray> decodeOneTimeQueue(QByteArrayView encoded) const;

    VaultService &m_vault;
    IdentityKeyStore &m_identityKeyStore;
    int m_nextSignedPrekeyId{1};
};

} // namespace vox::crypto

#endif // VOX_CRYPTO_PREKEYMANAGER_HPP
