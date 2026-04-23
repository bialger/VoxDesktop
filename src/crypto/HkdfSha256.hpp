#ifndef VOX_CRYPTO_HKDFSHA256_HPP
#define VOX_CRYPTO_HKDFSHA256_HPP

#include <QByteArray>
#include <QByteArrayView>

namespace vox::crypto {

QByteArray hkdfSha256(QByteArrayView ikm,
                      QByteArrayView salt,
                      QByteArrayView info,
                      qsizetype outputLen);

} // namespace vox::crypto

#endif // VOX_CRYPTO_HKDFSHA256_HPP
