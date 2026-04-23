#include "crypto/VaultService.hpp"

#include "crypto/CryptoHelpers.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <sodium.h>

namespace vox::crypto {
namespace {

QByteArray toB64(const QByteArray &bytes) {
  return bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

QByteArray fromB64(const QByteArray &bytes) {
  return QByteArray::fromBase64(bytes, QByteArray::Base64UrlEncoding);
}

constexpr quint64 kVaultOpsLimit = crypto_pwhash_OPSLIMIT_INTERACTIVE;
constexpr size_t kVaultMemLimit = crypto_pwhash_MEMLIMIT_INTERACTIVE;

} // namespace

VaultService::VaultService(QString vaultPath) : m_vaultPath(std::move(vaultPath)) {
}

bool VaultService::unlockWithPassword(const QString &password) {
  if (password.isEmpty()) {
    return false;
  }

  if (QFile::exists(m_vaultPath)) {
    return loadExistingVault(password);
  }

  m_salt = CryptoHelpers::randomBytes(crypto_pwhash_SALTBYTES);
  if (m_salt.isEmpty()) {
    return false;
  }

  m_vaultKey = CryptoHelpers::argon2idKdf(password.toUtf8(), m_salt, kVaultOpsLimit, kVaultMemLimit, 32);
  if (m_vaultKey.size() != 32) {
    return false;
  }

  m_unlocked = true;
  return saveVault();
}

bool VaultService::isUnlocked() const {
  return m_unlocked;
}

void VaultService::lock() {
  if (!m_vaultKey.isEmpty()) {
    sodium_memzero(m_vaultKey.data(), static_cast<size_t>(m_vaultKey.size()));
  }
  m_vaultKey.clear();
  m_secrets.clear();
  m_unlocked = false;
}

bool VaultService::storeSecret(const QString &name, QByteArray value) {
  if (!m_unlocked || name.isEmpty() || value.isEmpty()) {
    return false;
  }

  m_secrets.insert(name, std::move(value));
  return saveVault();
}

std::optional<QByteArray> VaultService::loadSecret(const QString &name) const {
  if (!m_unlocked || name.isEmpty()) {
    return std::nullopt;
  }

  const auto it = m_secrets.find(name);
  if (it == m_secrets.end()) {
    return std::nullopt;
  }
  return it.value();
}

bool VaultService::removeSecret(const QString &name) {
  if (!m_unlocked || name.isEmpty()) {
    return false;
  }
  if (!m_secrets.contains(name)) {
    return true;
  }
  m_secrets.remove(name);
  return saveVault();
}

bool VaultService::loadExistingVault(const QString &password) {
  QFile file(m_vaultPath);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }

  const auto doc = QJsonDocument::fromJson(file.readAll());
  const auto root = doc.object();

  m_salt = fromB64(root.value("salt").toString().toUtf8());
  const QByteArray nonce = fromB64(root.value("nonce").toString().toUtf8());
  const QByteArray ciphertext = fromB64(root.value("ciphertext").toString().toUtf8());

  m_vaultKey = CryptoHelpers::argon2idKdf(password.toUtf8(), m_salt, kVaultOpsLimit, kVaultMemLimit, 32);
  if (m_vaultKey.size() != 32) {
    return false;
  }

  const auto decrypted = CryptoHelpers::xchachaDecryptLocal(m_vaultKey, nonce, ciphertext, "vox-vault-v1");
  if (!decrypted.has_value()) {
    lock();
    return false;
  }

  const auto secretsDoc = QJsonDocument::fromJson(*decrypted);
  const auto secretsObj = secretsDoc.object();

  m_secrets.clear();
  for (auto it = secretsObj.begin(); it != secretsObj.end(); ++it) {
    m_secrets.insert(it.key(), fromB64(it.value().toString().toUtf8()));
  }

  m_unlocked = true;
  return true;
}

bool VaultService::saveVault() {
  if (!m_unlocked || m_vaultKey.size() != 32 || m_salt.size() != crypto_pwhash_SALTBYTES) {
    return false;
  }

  QJsonObject payloadObj;
  for (auto it = m_secrets.begin(); it != m_secrets.end(); ++it) {
    payloadObj.insert(it.key(), QString::fromUtf8(toB64(it.value())));
  }

  const QByteArray payload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);
  const QByteArray nonce = CryptoHelpers::randomBytes(crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  if (nonce.size() != crypto_aead_xchacha20poly1305_ietf_NPUBBYTES) {
    return false;
  }

  const QByteArray ciphertext = CryptoHelpers::xchachaEncryptLocal(m_vaultKey, nonce, payload, "vox-vault-v1");
  if (ciphertext.isEmpty()) {
    return false;
  }

  QJsonObject root;
  root.insert("version", 1);
  root.insert("salt", QString::fromUtf8(toB64(m_salt)));
  root.insert("nonce", QString::fromUtf8(toB64(nonce)));
  root.insert("ciphertext", QString::fromUtf8(toB64(ciphertext)));

  QSaveFile out(m_vaultPath);
  if (!out.open(QIODevice::WriteOnly)) {
    return false;
  }

  out.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
  return out.commit();
}

} // namespace vox::crypto
