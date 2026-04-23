#include "services/AuthService.hpp"

#include "crypto/CryptoHelpers.hpp"
#include "crypto/SyncCrypto.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QUuid>

namespace vox::services {
namespace {

QString toB64(const QByteArray &bytes) {
  return QString::fromUtf8(bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

QString sha256HexLower(const QString &input) {
  const QByteArray hash = QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Sha256);
  return QString::fromLatin1(hash.toHex());
}

} // namespace

AuthService::AuthService(network::AuthApi &authApi,
                         network::NetworkAccess &networkAccess,
                         storage::IAccountsRepository &accountsRepository,
                         crypto::VaultService &vaultService,
                         crypto::IdentityKeyStore &identityKeyStore,
                         crypto::PrekeyManager &prekeyManager,
                         QString serverId,
                         QString deviceId) :
    m_authApi(authApi), m_network(networkAccess), m_accountsRepository(accountsRepository), m_vault(vaultService),
    m_identity(identityKeyStore), m_prekeys(prekeyManager), m_serverId(std::move(serverId)),
    m_deviceId(std::move(deviceId)) {
}

bool AuthService::registerUser(const QString &username, const QString &passwordDerived) {
  if (username.isEmpty() || passwordDerived.isEmpty()) {
    return false;
  }

  const QString derived = sha256HexLower(passwordDerived);

  if (!m_identity.ensureIdentityKeys()) {
    return false;
  }

  const auto identity = m_identity.identityKeys();
  const auto signedPrekey = m_prekeys.rotateSignedPrekey();

  if (!identity.has_value() || !signedPrekey.has_value()) {
    return false;
  }

  const QByteArray syncMasterKey = crypto::CryptoHelpers::randomBytes(32);
  const auto wrappedSync = crypto::SyncCrypto::wrapSyncMasterKey(derived.toUtf8(), syncMasterKey);
  if (!wrappedSync.has_value()) {
    return false;
  }

  network::RegisterRequest request;
  request.username = username;
  request.passwordDerivedValue = derived;
  request.deviceId = m_deviceId;
  request.deviceLabel = "Vox Desktop";
  request.identityKeyPublic = toB64(identity->dhPublic);
  request.signedPrekeyPublic = toB64(signedPrekey->publicKey);
  request.signedPrekeySignature = toB64(signedPrekey->signature);
  request.wrappedSyncKey = toB64(wrappedSync->wrappedSyncKey);
  request.syncWrapSalt = toB64(wrappedSync->salt);
  request.syncWrapParams = wrappedSync->params.toJson();

  const auto response = m_authApi.registerUser(request);
  if (!response.ok || !response.data.has_value()) {
    return false;
  }

  return persistSession(username, *response.data);
}

bool AuthService::login(const QString &username, const QString &passwordDerived) {
  if (username.isEmpty() || passwordDerived.isEmpty()) {
    return false;
  }

  const QString derived = sha256HexLower(passwordDerived);

  network::LoginRequest request;
  request.username = username;
  request.passwordDerivedValue = derived;
  request.deviceId = m_deviceId;
  request.deviceLabel = "Vox Desktop";

  if (m_identity.ensureIdentityKeys()) {
    const auto identity = m_identity.identityKeys();
    const auto signedPrekey = m_prekeys.rotateSignedPrekey();
    if (identity.has_value() && signedPrekey.has_value()) {
      request.identityKeyPublic = toB64(identity->dhPublic);
      request.signedPrekeyPublic = toB64(signedPrekey->publicKey);
      request.signedPrekeySignature = toB64(signedPrekey->signature);
    }
  }

  const auto response = m_authApi.login(request);
  if (!response.ok || !response.data.has_value()) {
    return false;
  }

  return persistSession(username, *response.data);
}

bool AuthService::restoreSession() {
  const auto account = m_accountsRepository.activeAccount();
  if (!account.has_value()) {
    return false;
  }

  const auto refreshToken = m_vault.loadSecret("refresh_token/" + account->userId);
  if (!refreshToken.has_value()) {
    return false;
  }

  const auto refreshed =
      m_authApi.refresh(network::RefreshRequest{QString::fromUtf8(*refreshToken), account->activeDeviceId});
  if (!refreshed.ok || !refreshed.data.has_value()) {
    return false;
  }

  m_network.setBearerToken(refreshed.data->accessToken);

  m_context = AuthContext{account->userId,
                          account->username,
                          account->activeDeviceId,
                          refreshed.data->accessToken,
                          refreshed.data->refreshToken};

  m_vault.storeSecret("refresh_token/" + account->userId, refreshed.data->refreshToken.toUtf8());

  domain::Account updated = *account;
  updated.refreshTokenCiphertext = refreshed.data->refreshToken.toUtf8();
  updated.lastLogin = QDateTime::currentDateTimeUtc();

  return m_accountsRepository.upsertAccount(updated);
}

bool AuthService::logout() {
  if (!m_context.has_value()) {
    return true;
  }

  const auto result = m_authApi.logout();
  const auto active = m_accountsRepository.activeAccount();
  if (active.has_value()) {
    m_vault.removeSecret("refresh_token/" + active->userId);
    m_accountsRepository.deleteAccount(active->accountId);
  }
  m_network.setBearerToken({});
  m_context.reset();
  return result.ok;
}

std::optional<AuthContext> AuthService::context() const {
  return m_context;
}

bool AuthService::persistSession(const QString &username, const network::AuthSessionResponse &response) {
  m_network.setBearerToken(response.accessToken);

  m_context = AuthContext{response.userId, username, m_deviceId, response.accessToken, response.refreshToken};

  if (!m_vault.storeSecret("refresh_token/" + response.userId, response.refreshToken.toUtf8())) {
    return false;
  }

  domain::Account account;
  account.accountId = QUuid::createUuid().toString(QUuid::WithoutBraces);
  account.serverId = m_serverId;
  account.userId = response.userId;
  account.username = username;
  account.activeDeviceId = m_deviceId;
  account.refreshTokenCiphertext = response.refreshToken.toUtf8();
  account.lastLogin = QDateTime::currentDateTimeUtc();

  return m_accountsRepository.upsertAccount(account);
}

} // namespace vox::services
