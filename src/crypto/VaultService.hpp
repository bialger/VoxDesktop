#ifndef VOX_CRYPTO_VAULTSERVICE_HPP
#define VOX_CRYPTO_VAULTSERVICE_HPP

#include <QByteArray>
#include <QHash>
#include <QString>
#include <optional>

namespace vox::crypto {

class VaultService final {
public:
  explicit VaultService(QString vaultPath);

  bool unlockWithPassword(const QString &password);
  bool isUnlocked() const;
  void lock();

  bool storeSecret(const QString &name, const QByteArray &value);
  std::optional<QByteArray> loadSecret(const QString &name) const;
  bool removeSecret(const QString &name);

private:
  bool loadExistingVault(const QString &password);
  bool saveVault();

  QString m_vaultPath;
  QByteArray m_vaultKey;
  QByteArray m_salt;
  QHash<QString, QByteArray> m_secrets;
  bool m_unlocked{false};
};

} // namespace vox::crypto

#endif // VOX_CRYPTO_VAULTSERVICE_HPP
