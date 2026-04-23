#ifndef VOX_APP_SERVICELOCATOR_HPP
#define VOX_APP_SERVICELOCATOR_HPP

#include <memory>

#include "crypto/IdentityKeyStore.hpp"
#include "crypto/PrekeyManager.hpp"
#include "crypto/VaultService.hpp"
#include "network/AttachmentsApi.hpp"
#include "network/AuthApi.hpp"
#include "network/ConversationsApi.hpp"
#include "network/DirectoryApi.hpp"
#include "network/NetworkAccess.hpp"
#include "network/SyncApi.hpp"
#include "services/AttachmentService.hpp"
#include "services/AuthService.hpp"
#include "services/ConversationService.hpp"
#include "services/MessageSendService.hpp"
#include "services/NotificationService.hpp"
#include "services/SearchService.hpp"
#include "services/ServerDiscoveryService.hpp"
#include "services/SyncService.hpp"
#include "storage/Database.hpp"
#include "storage/FileStore.hpp"
#include "storage/Repositories.hpp"

namespace vox::app {

class ServiceLocator final {
public:
  bool initialize(const QString &appDataPath,
                  const QString &serverBaseUrl,
                  const QString &vaultPassword,
                  QString *error);

  services::AuthService *authService() const;
  services::ServerDiscoveryService *serverDiscoveryService() const;
  services::ConversationService *conversationService() const;
  services::MessageSendService *messageSendService() const;
  services::AttachmentService *attachmentService() const;
  services::SyncService *syncService() const;
  services::SearchService *searchService() const;
  services::NotificationService *notificationService() const;

  storage::IMessagesRepository *messagesRepository() const;
  network::DirectoryApi *directoryApi() const;
  network::ConversationsApi *conversationsApi() const;

private:
  std::unique_ptr<storage::Database> m_database;
  std::unique_ptr<storage::FileStore> m_fileStore;

  std::unique_ptr<crypto::VaultService> m_vault;
  std::unique_ptr<crypto::IdentityKeyStore> m_identityStore;
  std::unique_ptr<crypto::PrekeyManager> m_prekeyManager;

  std::unique_ptr<network::NetworkAccess> m_networkAccess;
  std::unique_ptr<network::AuthApi> m_authApi;
  std::unique_ptr<network::DirectoryApi> m_directoryApi;
  std::unique_ptr<network::ConversationsApi> m_conversationsApi;
  std::unique_ptr<network::AttachmentsApi> m_attachmentsApi;
  std::unique_ptr<network::SyncApi> m_syncApi;

  std::unique_ptr<storage::SqlAccountsRepository> m_accountsRepository;
  std::unique_ptr<storage::SqlConversationsRepository> m_conversationsRepository;
  std::unique_ptr<storage::SqlMessagesRepository> m_messagesRepository;
  std::unique_ptr<storage::SqlDevicesRepository> m_devicesRepository;
  std::unique_ptr<storage::SqlSyncRepository> m_syncRepository;
  std::unique_ptr<storage::SqlJobsRepository> m_jobsRepository;
  std::unique_ptr<storage::SqlSearchRepository> m_searchRepository;
  std::unique_ptr<storage::SqlTrustRepository> m_trustRepository;

  std::unique_ptr<services::AuthService> m_authService;
  std::unique_ptr<services::ServerDiscoveryService> m_serverDiscoveryService;
  std::unique_ptr<services::ConversationService> m_conversationService;
  std::unique_ptr<services::MessageSendService> m_messageSendService;
  std::unique_ptr<services::AttachmentService> m_attachmentService;
  std::unique_ptr<services::SyncService> m_syncService;
  std::unique_ptr<services::SearchService> m_searchService;
  std::unique_ptr<services::NotificationService> m_notificationService;
};

} // namespace vox::app

#endif // VOX_APP_SERVICELOCATOR_HPP
