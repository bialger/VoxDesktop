#include "crypto/HkdfSha256.hpp"

#include "crypto/CryptoHelpers.hpp"
#include "crypto/CryptoSizes.hpp"

namespace vox::crypto {

QByteArray hkdfSha256(QByteArrayView ikm, QByteArrayView salt, QByteArrayView info, qsizetype outputLen) {
  if (outputLen <= 0 || ikm.empty()) {
    return {};
  }

  QByteArray normalized_salt;
  if (salt.empty()) {
    normalized_salt = QByteArray(kHkdfDefaultSaltBytes, '\0');
    salt = normalized_salt;
  }

  const QByteArray prk = CryptoHelpers::hmacSha256(salt, ikm);
  if (prk.isEmpty()) {
    return {};
  }

  QByteArray out;
  out.reserve(outputLen);

  QByteArray t;
  quint8 counter = 1;
  while (out.size() < outputLen) {
    QByteArray block_input;
    block_input.reserve(t.size() + info.size() + 1);
    block_input.append(t);
    block_input.append(info.data(), info.size());
    block_input.append(static_cast<char>(counter));

    t = CryptoHelpers::hmacSha256(prk, block_input);
    if (t.isEmpty()) {
      return {};
    }

    out.append(t);
    ++counter;
    if (counter == 0) {
      return {};
    }
  }

  out.truncate(outputLen);
  return out;
}

} // namespace vox::crypto
