#include "crypto/DoubleRatchetSession.hpp"

#include "crypto/CryptoHelpers.hpp"
#include "crypto/CryptoSizes.hpp"
#include "crypto/HkdfSha256.hpp"

#include <QtEndian>
#include <cstring>

namespace vox::crypto {
namespace {

QByteArray IntLabel(const char *prefix, int value) {
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
  if (bytes.size() < kRatchetMinSerializedBytes) {
    return std::nullopt;
  }

  quint32 be_counter = 0;
  memcpy(&be_counter, bytes.data(), 4);

  RatchetMessage msg;
  msg.counter = static_cast<int>(qFromBigEndian(be_counter));
  msg.nonce = QByteArray(bytes.data() + kRatchetCounterBytes, kChaCha20NonceBytes);
  msg.ciphertext = QByteArray(bytes.data() + kRatchetHeaderBytes, bytes.size() - kRatchetHeaderBytes);
  return msg;
}

DoubleRatchetSession DoubleRatchetSession::fromSharedSecret(QByteArray sharedSecret, QByteArray associatedData) {
  DoubleRatchetSession session;
  session.m_rootKey = std::move(sharedSecret);
  session.m_associatedData = std::move(associatedData);
  const QByteArray base_chain_key =
      hkdfSha256(session.m_rootKey, "vox-ratchet-chain", "vox-e2ee-v1/chain", kChaCha20KeyBytes);
  session.m_sendChainKey = base_chain_key;
  session.m_receiveChainKey = base_chain_key;
  return session;
}

std::optional<QByteArray> DoubleRatchetSession::encrypt(QByteArrayView plaintext, QByteArrayView ad) {
  const QByteArray message_key = nextMessageKey(true, m_sendCounter);
  if (message_key.isEmpty()) {
    return std::nullopt;
  }

  const QByteArray nonce = nextNonce(message_key);
  QByteArray local_ad = m_associatedData;
  local_ad.append(ad.data(), ad.size());

  const QByteArray ciphertext = CryptoHelpers::aeadEncrypt(message_key, nonce, plaintext, local_ad);
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

  const QByteArray message_key = nextMessageKey(false, parsed->counter);
  QByteArray local_ad = m_associatedData;
  local_ad.append(ad.data(), ad.size());

  auto plaintext = CryptoHelpers::aeadDecrypt(message_key, parsed->nonce, parsed->ciphertext, local_ad);
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
  return CryptoHelpers::hmacSha256(chain, IntLabel("msg", counter));
}

QByteArray DoubleRatchetSession::nextNonce(const QByteArray &messageKey) const {
  const QByteArray digest = CryptoHelpers::hmacSha256(messageKey, "nonce");
  return digest.left(kChaCha20NonceBytes);
}

void DoubleRatchetSession::advanceChain(bool sending, int counter) {
  QByteArray &chain = sending ? m_sendChainKey : m_receiveChainKey;
  chain = CryptoHelpers::hmacSha256(chain, IntLabel("chain", counter));
}

} // namespace vox::crypto
