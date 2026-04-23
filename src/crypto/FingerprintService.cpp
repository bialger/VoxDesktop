#include "crypto/FingerprintService.hpp"

#include "crypto/CryptoHelpers.hpp"

namespace vox::crypto {

QString FingerprintService::fingerprint(QByteArrayView signingPublicKey, QByteArrayView dhPublicKey) {
  QByteArray data;
  data.reserve(signingPublicKey.size() + dhPublicKey.size());
  data.append(signingPublicKey.data(), signingPublicKey.size());
  data.append(dhPublicKey.data(), dhPublicKey.size());

  const QByteArray digest = CryptoHelpers::sha256(data).toHex();
  QString grouped;
  grouped.reserve(digest.size() + digest.size() / 4);

  for (int i = 0; i < digest.size(); ++i) {
    grouped.append(QChar::fromLatin1(digest.at(i)).toUpper());
    if ((i + 1) % 4 == 0 && i + 1 < digest.size()) {
      grouped.append(':');
    }
  }
  return grouped;
}

} // namespace vox::crypto
