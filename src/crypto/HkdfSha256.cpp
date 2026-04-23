#include "crypto/HkdfSha256.hpp"

#include "crypto/CryptoHelpers.hpp"

namespace vox::crypto {

QByteArray hkdfSha256(QByteArrayView ikm, QByteArrayView salt, QByteArrayView info, qsizetype outputLen) {
  if (outputLen <= 0 || ikm.empty()) {
    return {};
  }

  QByteArray normalizedSalt;
  if (salt.empty()) {
    normalizedSalt = QByteArray(32, '\0');
    salt = normalizedSalt;
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
    QByteArray blockInput;
    blockInput.reserve(t.size() + info.size() + 1);
    blockInput.append(t);
    blockInput.append(info.data(), info.size());
    blockInput.append(static_cast<char>(counter));

    t = CryptoHelpers::hmacSha256(prk, blockInput);
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
