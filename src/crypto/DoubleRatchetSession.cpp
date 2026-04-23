#include "crypto/DoubleRatchetSession.hpp"

#include "crypto/CryptoHelpers.hpp"
#include "crypto/HkdfSha256.hpp"

#include <QtEndian>
#include <cstring>

namespace vox::crypto {
namespace {

QByteArray intLabel(const char *prefix, int value) {
  QByteArray out(prefix);
  out.append(':');
  out.append(QByteArray::number(value));
  return out;
}

} // namespace

QByteArray RatchetMessage::serialize() const {
  QByteArray out;
  out.reserve(4 + nonce.size() + ciphertext.size());

  const quint32 be = qToBigEndian(static_cast<quint32>(counter));
  out.append(reinterpret_cast<const char *>(&be), 4);
  out.append(nonce);
  out.append(ciphertext);
  return out;
}

std::optional<RatchetMessage> RatchetMessage::deserialize(QByteArrayView bytes) {
  if (bytes.size() < 4 + 12 + 16) {
    return std::nullopt;
  }

  quint32 beCounter = 0;
  memcpy(&beCounter, bytes.data(), 4);

  RatchetMessage msg;
  msg.counter = static_cast<int>(qFromBigEndian(beCounter));
  msg.nonce = QByteArray(bytes.data() + 4, 12);
  msg.ciphertext = QByteArray(bytes.data() + 16, bytes.size() - 16);
  return msg;
}

DoubleRatchetSession DoubleRatchetSession::fromSharedSecret(QByteArray sharedSecret, QByteArray associatedData) {
  DoubleRatchetSession session;
  session.m_rootKey = std::move(sharedSecret);
  session.m_associatedData = std::move(associatedData);
  const QByteArray baseChainKey = hkdfSha256(session.m_rootKey, "vox-ratchet-chain", "vox-e2ee-v1/chain", 32);
  session.m_sendChainKey = baseChainKey;
  session.m_receiveChainKey = baseChainKey;
  return session;
}

std::optional<QByteArray> DoubleRatchetSession::encrypt(QByteArrayView plaintext, QByteArrayView ad) {
  const QByteArray messageKey = nextMessageKey(true, m_sendCounter);
  if (messageKey.isEmpty()) {
    return std::nullopt;
  }

  const QByteArray nonce = nextNonce(messageKey);
  QByteArray localAd = m_associatedData;
  localAd.append(ad.data(), ad.size());

  const QByteArray ciphertext = CryptoHelpers::aeadEncrypt(messageKey, nonce, plaintext, localAd);
  if (ciphertext.isEmpty()) {
    return std::nullopt;
  }

  RatchetMessage msg{m_sendCounter, nonce, ciphertext};
  advanceChain(true, m_sendCounter);
  ++m_sendCounter;
  return msg.serialize();
}

std::optional<QByteArray> DoubleRatchetSession::decrypt(QByteArrayView serializedMessage, QByteArrayView ad) {
  const auto parsed = RatchetMessage::deserialize(serializedMessage);
  if (!parsed.has_value() || parsed->counter < m_receiveCounter) {
    return std::nullopt;
  }

  while (m_receiveCounter < parsed->counter) {
    advanceChain(false, m_receiveCounter);
    ++m_receiveCounter;
  }

  const QByteArray messageKey = nextMessageKey(false, parsed->counter);
  QByteArray localAd = m_associatedData;
  localAd.append(ad.data(), ad.size());

  const auto plaintext = CryptoHelpers::aeadDecrypt(messageKey, parsed->nonce, parsed->ciphertext, localAd);
  if (!plaintext.has_value()) {
    return std::nullopt;
  }

  advanceChain(false, parsed->counter);
  m_receiveCounter = parsed->counter + 1;
  return plaintext;
}

int DoubleRatchetSession::sendCounter() const {
  return m_sendCounter;
}

int DoubleRatchetSession::receiveCounter() const {
  return m_receiveCounter;
}

QByteArray DoubleRatchetSession::nextMessageKey(bool sending, int counter) const {
  const QByteArray &chain = sending ? m_sendChainKey : m_receiveChainKey;
  return CryptoHelpers::hmacSha256(chain, intLabel("msg", counter));
}

QByteArray DoubleRatchetSession::nextNonce(const QByteArray &messageKey) const {
  const QByteArray digest = CryptoHelpers::hmacSha256(messageKey, "nonce");
  return digest.left(12);
}

void DoubleRatchetSession::advanceChain(bool sending, int counter) {
  QByteArray &chain = sending ? m_sendChainKey : m_receiveChainKey;
  chain = CryptoHelpers::hmacSha256(chain, intLabel("chain", counter));
}

} // namespace vox::crypto
