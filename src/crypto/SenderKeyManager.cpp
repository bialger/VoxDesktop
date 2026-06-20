#include "crypto/SenderKeyManager.hpp"

#include "crypto/CryptoHelpers.hpp"
#include "crypto/CryptoSizes.hpp"

namespace vox::crypto {

int SenderKeyManager::currentEpoch(const QString &conversationId) const {
  return m_conversations.value(conversationId).currentEpoch;
}

std::optional<QByteArray> SenderKeyManager::keyForEpoch(const QString &conversationId, int epoch) const {
  const auto conversation_it = m_conversations.find(conversationId);
  if (conversation_it == m_conversations.end()) {
    return std::nullopt;
  }

  const auto key_it = conversation_it->keys.find(epoch);
  if (key_it == conversation_it->keys.end()) {
    return std::nullopt;
  }

  return key_it.value();
}

QByteArray SenderKeyManager::rotateEpoch(const QString &conversationId) {
  auto &state = m_conversations[conversationId];
  ++state.currentEpoch;
  const QByteArray new_key = CryptoHelpers::randomBytes(kChaCha20KeyBytes);
  state.keys.insert(state.currentEpoch, new_key);
  return new_key;
}

bool SenderKeyManager::importEpochKey(const QString &conversationId, int epoch, const QByteArray &key) {
  if (epoch <= 0 || key.size() != kChaCha20KeyBytes) {
    return false;
  }

  auto &state = m_conversations[conversationId];
  state.currentEpoch = std::max(state.currentEpoch, epoch);
  state.keys.insert(epoch, key);
  return true;
}

} // namespace vox::crypto
