#include "app/ServiceLocator.hpp"

#include "crypto/CryptoHelpers.hpp"
#include "crypto/CryptoSizes.hpp"
#include "crypto/HkdfSha256.hpp"
#include "storage/Migrations.hpp"

#include <QDir>
#include <QSettings>
#include <QUuid>

namespace vox::app {

bool ServiceLocator::initialize(const QString &appDataPath,
                                const QString &serverBaseUrl,
                                const QString &vaultPassword,
                                QString *error) {
  QDir dir(appDataPath);
  if (!dir.exists() && !dir.mkpath(".")) {
    if (error != nullptr) {
      *error = "Failed to create app data directory";
    }
    return false;
  }

  m_vault = std::make_unique<crypto::VaultService>(dir.filePath("vault.json"));
  if (!m_vault->unlockWithPassword(vaultPassword)) {
    if (error != nullptr) {
      *error = "Failed to unlock local vault";
    }
    return false;
  }

  m_identityStore = std::make_unique<crypto::IdentityKeyStore>(*m_vault);
  m_prekeyManager = std::make_unique<crypto::PrekeyManager>(*m_vault, *m_identityStore);

  m_database = std::make_unique<storage::Database>();
  if (!m_database->open(dir.filePath("vox.sqlite"))) {
    if (error != nullptr) {
      *error = m_database->lastError();
    }
    return false;
  }

  QString migration_error;
  if (!storage::Migrations::run(m_database->database(), &migration_error)) {
    if (error != nullptr) {
      *error = QString("Database migration failed: %1").arg(migration_error);
    }
    return false;
  }

  m_fileStore = std::make_unique<storage::FileStore>(dir.filePath("files"));
  if (!m_fileStore->initialize()) {
    if (error != nullptr) {
      *error = "Failed to initialize file storage";
    }
    return false;
  }

  m_networkAccess = std::make_unique<network::NetworkAccess>(serverBaseUrl);
  m_authApi = std::make_unique<network::AuthApi>(*m_networkAccess);
  m_directoryApi = std::make_unique<network::DirectoryApi>(*m_networkAccess);
  m_conversationsApi = std::make_unique<network::ConversationsApi>(*m_networkAccess);
  m_attachmentsApi = std::make_unique<network::AttachmentsApi>(*m_networkAccess);
  m_syncApi = std::make_unique<network::SyncApi>(*m_networkAccess);

  const auto db = m_database->database();
  m_accountsRepository = std::make_unique<storage::SqlAccountsRepository>(db);
  m_conversationsRepository = std::make_unique<storage::SqlConversationsRepository>(db);
  m_messagesRepository = std::make_unique<storage::SqlMessagesRepository>(db);
  m_devicesRepository = std::make_unique<storage::SqlDevicesRepository>(db);
  m_syncRepository = std::make_unique<storage::SqlSyncRepository>(db);
  m_jobsRepository = std::make_unique<storage::SqlJobsRepository>(db);
  m_searchRepository = std::make_unique<storage::SqlSearchRepository>(db);
  m_trustRepository = std::make_unique<storage::SqlTrustRepository>(db);

  QSettings settings;
  QString device_id = settings.value("vox/device_id").toString();
  if (device_id.isEmpty()) {
    device_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    settings.setValue("vox/device_id", device_id);
  }

  m_authService = std::make_unique<services::AuthService>(*m_authApi,
                                                          *m_networkAccess,
                                                          *m_accountsRepository,
                                                          *m_vault,
                                                          *m_identityStore,
                                                          *m_prekeyManager,
                                                          "default",
                                                          device_id);

  m_serverDiscoveryService = std::make_unique<services::ServerDiscoveryService>(*m_networkAccess);
  m_conversationService =
      std::make_unique<services::ConversationService>(*m_conversationsApi, *m_directoryApi, *m_conversationsRepository);
  m_messageSendService =
      std::make_unique<services::MessageSendService>(*m_conversationsApi, *m_messagesRepository, *m_jobsRepository);
  m_attachmentService = std::make_unique<services::AttachmentService>(*m_attachmentsApi, *m_fileStore);
  m_syncService = std::make_unique<services::SyncService>(*m_syncApi, *m_syncRepository);

  auto search_key = m_vault->loadSecret("search_index_key");
  if (!search_key.has_value()) {
    const QByteArray seed = crypto::CryptoHelpers::randomBytes(crypto::kChaCha20KeyBytes);
    search_key = crypto::hkdfSha256(seed, "vox-search", "vox/search-index-key", crypto::kChaCha20KeyBytes);
    m_vault->storeSecret("search_index_key", *search_key);
  }
  m_searchService = std::make_unique<services::SearchService>(*m_searchRepository, *search_key);

  m_notificationService = std::make_unique<services::NotificationService>(false);

  return true;
}

services::AuthService *ServiceLocator::authService() const {
  return m_authService.get();
}

services::ServerDiscoveryService *ServiceLocator::serverDiscoveryService() const {
  return m_serverDiscoveryService.get();
}

services::ConversationService *ServiceLocator::conversationService() const {
  return m_conversationService.get();
}

services::MessageSendService *ServiceLocator::messageSendService() const {
  return m_messageSendService.get();
}

services::AttachmentService *ServiceLocator::attachmentService() const {
  return m_attachmentService.get();
}

services::SyncService *ServiceLocator::syncService() const {
  return m_syncService.get();
}

services::SearchService *ServiceLocator::searchService() const {
  return m_searchService.get();
}

services::NotificationService *ServiceLocator::notificationService() const {
  return m_notificationService.get();
}

storage::IMessagesRepository *ServiceLocator::messagesRepository() const {
  return m_messagesRepository.get();
}

network::DirectoryApi *ServiceLocator::directoryApi() const {
  return m_directoryApi.get();
}

network::ConversationsApi *ServiceLocator::conversationsApi() const {
  return m_conversationsApi.get();
}

} // namespace vox::app
