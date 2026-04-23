#include "crypto/SenderKeyManager.hpp"

#include "crypto/CryptoHelpers.hpp"

namespace vox::crypto {

int SenderKeyManager::currentEpoch(const QString &conversationId) const {
    return m_conversations.value(conversationId).currentEpoch;
}

std::optional<QByteArray> SenderKeyManager::keyForEpoch(const QString &conversationId, int epoch) const {
    const auto conversationIt = m_conversations.find(conversationId);
    if (conversationIt == m_conversations.end()) {
        return std::nullopt;
    }

    const auto keyIt = conversationIt->keys.find(epoch);
    if (keyIt == conversationIt->keys.end()) {
        return std::nullopt;
    }

    return keyIt.value();
}

QByteArray SenderKeyManager::rotateEpoch(const QString &conversationId) {
    auto &state = m_conversations[conversationId];
    ++state.currentEpoch;
    const QByteArray newKey = CryptoHelpers::randomBytes(32);
    state.keys.insert(state.currentEpoch, newKey);
    return newKey;
}

bool SenderKeyManager::importEpochKey(const QString &conversationId, int epoch, QByteArray key) {
    if (epoch <= 0 || key.size() != 32) {
        return false;
    }

    auto &state = m_conversations[conversationId];
    state.currentEpoch = std::max(state.currentEpoch, epoch);
    state.keys.insert(epoch, std::move(key));
    return true;
}

} // namespace vox::crypto
