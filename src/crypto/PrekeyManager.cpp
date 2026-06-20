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

QByteArray ToB64(const QByteArray &bytes) {
  return bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

QByteArray FromB64(const QByteArray &bytes) {
  return QByteArray::fromBase64(bytes, QByteArray::Base64UrlEncoding);
}

} // namespace

PrekeyManager::PrekeyManager(VaultService &vault, IdentityKeyStore &identityKeyStore) :
    m_vault(vault), m_identityKeyStore(identityKeyStore) {
}

std::optional<SignedPrekey> PrekeyManager::rotateSignedPrekey() {
  if (!m_vault.isUnlocked() || !ensureSodiumInitialized()) {
    return std::nullopt;
  }

  const auto identity = m_identityKeyStore.identityKeys();
  if (!identity.has_value()) {
    return std::nullopt;
  }

  QByteArray prekey_pub(crypto_kx_PUBLICKEYBYTES, Qt::Uninitialized);
  QByteArray prekey_priv(crypto_kx_SECRETKEYBYTES, Qt::Uninitialized);
  if (crypto_kx_keypair(reinterpret_cast<unsigned char *>(prekey_pub.data()),
                        reinterpret_cast<unsigned char *>(prekey_priv.data())) != 0) {
    return std::nullopt;
  }

  const QByteArray signature = CryptoHelpers::ed25519Sign(identity->signingPrivate, prekey_pub);
  if (signature.isEmpty()) {
    return std::nullopt;
  }

  SignedPrekey signed_prekey{
      .keyId = m_nextSignedPrekeyId++, .publicKey = prekey_pub, .privateKey = prekey_priv, .signature = signature};

  if (!m_vault.storeSecret(kSignedPrekeyIdName, QByteArray::number(signed_prekey.keyId)) ||
      !m_vault.storeSecret(kSignedPrekeyPubName, signed_prekey.publicKey) ||
      !m_vault.storeSecret(kSignedPrekeyPrivName, signed_prekey.privateKey) ||
      !m_vault.storeSecret(kSignedPrekeySigName, signed_prekey.signature)) {
    return std::nullopt;
  }

  return signed_prekey;
}

QVector<QByteArray> PrekeyManager::publishOneTimePrekeys(int count) {
  QVector<QByteArray> created;
  if (count <= 0 || !m_vault.isUnlocked() || !ensureSodiumInitialized()) {
    return created;
  }

  auto queue = decodeOneTimeQueue(m_vault.loadSecret(kOneTimeQueueName).value_or(QByteArray{}));
  created.reserve(count);

  for (int i = 0; i < count; ++i) {
    QByteArray prekey_pub(crypto_kx_PUBLICKEYBYTES, Qt::Uninitialized);
    QByteArray prekey_priv(crypto_kx_SECRETKEYBYTES, Qt::Uninitialized);
    if (crypto_kx_keypair(reinterpret_cast<unsigned char *>(prekey_pub.data()),
                          reinterpret_cast<unsigned char *>(prekey_priv.data())) != 0) {
      break;
    }

    QJsonObject item;
    item.insert("pub", QString::fromUtf8(ToB64(prekey_pub)));
    item.insert("priv", QString::fromUtf8(ToB64(prekey_priv)));

    queue.push_back(QJsonDocument(item).toJson(QJsonDocument::Compact));
    created.push_back(prekey_pub);
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
  return FromB64(obj.value("priv").toString().toUtf8());
}

QByteArray PrekeyManager::encodeOneTimeQueue(const QVector<QByteArray> &keys) const {
  QJsonArray arr;
  for (const auto &key : keys) {
    arr.append(QString::fromUtf8(ToB64(key)));
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
    out.push_back(FromB64(value.toString().toUtf8()));
  }
  return out;
}

} // namespace vox::crypto
