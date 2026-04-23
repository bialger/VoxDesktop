#ifndef VOX_SERVICES_AUTHSERVICE_HPP
#define VOX_SERVICES_AUTHSERVICE_HPP

#include <QString>

#include "crypto/IdentityKeyStore.hpp"
#include "crypto/PrekeyManager.hpp"
#include "crypto/VaultService.hpp"
#include "network/AuthApi.hpp"
#include "network/NetworkAccess.hpp"
#include "services/Interfaces.hpp"
#include "storage/Repositories.hpp"

namespace vox::services {

class AuthService final : public IAuthService {
public:
    AuthService(network::AuthApi &authApi,
                network::NetworkAccess &networkAccess,
                storage::IAccountsRepository &accountsRepository,
                crypto::VaultService &vaultService,
                crypto::IdentityKeyStore &identityKeyStore,
                crypto::PrekeyManager &prekeyManager,
                QString serverId,
                QString deviceId);

    bool registerUser(const QString &username, const QString &passwordDerived) override;
    bool login(const QString &username, const QString &passwordDerived) override;
    bool restoreSession() override;
    bool logout() override;
    std::optional<AuthContext> context() const override;

private:
    bool persistSession(const QString &username, const network::AuthSessionResponse &response);

    network::AuthApi &m_authApi;
    network::NetworkAccess &m_network;
    storage::IAccountsRepository &m_accountsRepository;
    crypto::VaultService &m_vault;
    crypto::IdentityKeyStore &m_identity;
    crypto::PrekeyManager &m_prekeys;
    QString m_serverId;
    QString m_deviceId;
    std::optional<AuthContext> m_context;
};

} // namespace vox::services

#endif // VOX_SERVICES_AUTHSERVICE_HPP
