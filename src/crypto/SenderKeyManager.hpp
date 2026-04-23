#ifndef VOX_CRYPTO_SENDERKEYMANAGER_HPP
#define VOX_CRYPTO_SENDERKEYMANAGER_HPP

#include <QByteArray>
#include <QHash>
#include <QString>
#include <optional>

namespace vox::crypto {

class SenderKeyManager final {
public:
    int currentEpoch(const QString &conversationId) const;
    std::optional<QByteArray> keyForEpoch(const QString &conversationId, int epoch) const;

    QByteArray rotateEpoch(const QString &conversationId);
    bool importEpochKey(const QString &conversationId, int epoch, QByteArray key);

private:
    struct EpochState {
        int currentEpoch{0};
        QHash<int, QByteArray> keys;
    };

    QHash<QString, EpochState> m_conversations;
};

} // namespace vox::crypto

#endif // VOX_CRYPTO_SENDERKEYMANAGER_HPP
