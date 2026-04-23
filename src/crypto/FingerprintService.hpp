#ifndef VOX_CRYPTO_FINGERPRINTSERVICE_HPP
#define VOX_CRYPTO_FINGERPRINTSERVICE_HPP

#include <QByteArray>
#include <QByteArrayView>
#include <QString>

namespace vox::crypto {

class FingerprintService final {
public:
    static QString fingerprint(QByteArrayView signingPublicKey, QByteArrayView dhPublicKey);
};

} // namespace vox::crypto

#endif // VOX_CRYPTO_FINGERPRINTSERVICE_HPP
