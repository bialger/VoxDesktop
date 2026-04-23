#ifndef VOX_CRYPTO_DOUBLERATCHETSESSION_HPP
#define VOX_CRYPTO_DOUBLERATCHETSESSION_HPP

#include <QByteArray>
#include <QByteArrayView>
#include <optional>

namespace vox::crypto {

struct RatchetMessage {
    int counter{0};
    QByteArray nonce;
    QByteArray ciphertext;

    QByteArray serialize() const;
    static std::optional<RatchetMessage> deserialize(QByteArrayView bytes);
};

class DoubleRatchetSession final {
public:
    static DoubleRatchetSession fromSharedSecret(QByteArray sharedSecret, QByteArray associatedData);

    std::optional<QByteArray> encrypt(QByteArrayView plaintext, QByteArrayView ad = {});
    std::optional<QByteArray> decrypt(QByteArrayView serializedMessage, QByteArrayView ad = {});

    int sendCounter() const;
    int receiveCounter() const;

private:
    QByteArray nextMessageKey(bool sending, int counter) const;
    QByteArray nextNonce(const QByteArray &messageKey) const;
    void advanceChain(bool sending, int counter);

    QByteArray m_rootKey;
    QByteArray m_sendChainKey;
    QByteArray m_receiveChainKey;
    QByteArray m_associatedData;
    int m_sendCounter{0};
    int m_receiveCounter{0};
};

} // namespace vox::crypto

#endif // VOX_CRYPTO_DOUBLERATCHETSESSION_HPP
