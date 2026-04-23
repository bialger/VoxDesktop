#include "crypto/PrekeyManager.hpp"

#include "crypto/CryptoHelpers.hpp"
#include "crypto/SodiumInit.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <sodium.h>

namespace vox::crypto {
namespace {
constexpr auto kSignedPrekeyIdName = "signed_prekey_id";
constexpr auto kSignedPrekeyPubName = "signed_prekey_pub";
constexpr auto kSignedPrekeyPrivName = "signed_prekey_priv";
constexpr auto kSignedPrekeySigName = "signed_prekey_sig";
constexpr auto kOneTimeQueueName = "one_time_prekeys";

QByteArray toB64(const QByteArray &bytes) {
    return bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

QByteArray fromB64(const QByteArray &bytes) {
    return QByteArray::fromBase64(bytes, QByteArray::Base64UrlEncoding);
}

} // namespace

PrekeyManager::PrekeyManager(VaultService &vault, IdentityKeyStore &identityKeyStore)
    : m_vault(vault), m_identityKeyStore(identityKeyStore) {}

std::optional<SignedPrekey> PrekeyManager::rotateSignedPrekey() {
    if (!m_vault.isUnlocked() || !ensureSodiumInitialized()) {
        return std::nullopt;
    }

    const auto identity = m_identityKeyStore.identityKeys();
    if (!identity.has_value()) {
        return std::nullopt;
    }

    QByteArray prekeyPub(crypto_kx_PUBLICKEYBYTES, Qt::Uninitialized);
    QByteArray prekeyPriv(crypto_kx_SECRETKEYBYTES, Qt::Uninitialized);
    if (crypto_kx_keypair(reinterpret_cast<unsigned char *>(prekeyPub.data()),
                          reinterpret_cast<unsigned char *>(prekeyPriv.data())) != 0) {
        return std::nullopt;
    }

    const QByteArray signature = CryptoHelpers::ed25519Sign(identity->signingPrivate, prekeyPub);
    if (signature.isEmpty()) {
        return std::nullopt;
    }

    const SignedPrekey signedPrekey{m_nextSignedPrekeyId++, prekeyPub, prekeyPriv, signature};

    if (!m_vault.storeSecret(kSignedPrekeyIdName, QByteArray::number(signedPrekey.keyId)) ||
        !m_vault.storeSecret(kSignedPrekeyPubName, signedPrekey.publicKey) ||
        !m_vault.storeSecret(kSignedPrekeyPrivName, signedPrekey.privateKey) ||
        !m_vault.storeSecret(kSignedPrekeySigName, signedPrekey.signature)) {
        return std::nullopt;
    }

    return signedPrekey;
}

QVector<QByteArray> PrekeyManager::publishOneTimePrekeys(int count) {
    QVector<QByteArray> created;
    if (count <= 0 || !m_vault.isUnlocked() || !ensureSodiumInitialized()) {
        return created;
    }

    auto queue = decodeOneTimeQueue(m_vault.loadSecret(kOneTimeQueueName).value_or(QByteArray{}));
    created.reserve(count);

    for (int i = 0; i < count; ++i) {
        QByteArray prekeyPub(crypto_kx_PUBLICKEYBYTES, Qt::Uninitialized);
        QByteArray prekeyPriv(crypto_kx_SECRETKEYBYTES, Qt::Uninitialized);
        if (crypto_kx_keypair(reinterpret_cast<unsigned char *>(prekeyPub.data()),
                              reinterpret_cast<unsigned char *>(prekeyPriv.data())) != 0) {
            break;
        }

        QJsonObject item;
        item.insert("pub", QString::fromUtf8(toB64(prekeyPub)));
        item.insert("priv", QString::fromUtf8(toB64(prekeyPriv)));

        queue.push_back(QJsonDocument(item).toJson(QJsonDocument::Compact));
        created.push_back(prekeyPub);
    }

    m_vault.storeSecret(kOneTimeQueueName, encodeOneTimeQueue(queue));
    return created;
}

std::optional<QByteArray> PrekeyManager::consumeOneTimePrekey() {
    auto queue = decodeOneTimeQueue(m_vault.loadSecret(kOneTimeQueueName).value_or(QByteArray{}));
    if (queue.isEmpty()) {
        return std::nullopt;
    }

    const QByteArray entry = queue.takeFirst();
    m_vault.storeSecret(kOneTimeQueueName, encodeOneTimeQueue(queue));

    const auto obj = QJsonDocument::fromJson(entry).object();
    return fromB64(obj.value("priv").toString().toUtf8());
}

QByteArray PrekeyManager::encodeOneTimeQueue(const QVector<QByteArray> &keys) const {
    QJsonArray arr;
    for (const auto &key : keys) {
        arr.append(QString::fromUtf8(toB64(key)));
    }
    return QJsonDocument(arr).toJson(QJsonDocument::Compact);
}

QVector<QByteArray> PrekeyManager::decodeOneTimeQueue(QByteArrayView encoded) const {
    QVector<QByteArray> out;
    if (encoded.empty()) {
        return out;
    }

    const auto arr = QJsonDocument::fromJson(encoded.toByteArray()).array();
    out.reserve(arr.size());
    for (const auto &value : arr) {
        out.push_back(fromB64(value.toString().toUtf8()));
    }
    return out;
}

} // namespace vox::crypto
