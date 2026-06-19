#include <QDateTime>
#include <QSqlDatabase>
#include <gtest/gtest.h>

#include "crypto/CryptoSizes.hpp"
#include "crypto/IdentityKeyStore.hpp"
#include "crypto/PrekeyManager.hpp"
#include "crypto/VaultService.hpp"
#include "network/AuthApi.hpp"
#include "network/ConversationsApi.hpp"
#include "network/NetworkAccess.hpp"
#include "services/AuthService.hpp"
#include "services/MessageSendService.hpp"
#include "services/SearchService.hpp"
#include "storage/Database.hpp"
#include "storage/Migrations.hpp"
#include "storage/Repositories.hpp"
#include "tests/integration/FakeVoxServer.hpp"
#include "tests/test_suites/ProjectUnitTestSuite.hpp"

namespace {

class InMemoryAccountsRepository final : public vox::storage::IAccountsRepository {
public:
  bool upsertAccount(const vox::domain::Account &account) override {
    m_account_ = account;
    return true;
  }

  [[nodiscard]] std::optional<vox::domain::Account> activeAccount() const override {
    return m_account_;
  }

  bool deleteAccount(const QString &accountId) override {
    if (!m_account_.has_value()) {
      return true;
    }
    if (m_account_->accountId != accountId) {
      return false;
    }
    m_account_.reset();
    return true;
  }

private:
  std::optional<vox::domain::Account> m_account_;
};

} // namespace

TEST_F(ProjectUnitTestSuite, AuthServiceRegisterAndRestoreSessionPositive) {
  FakeVoxServer fake_server;

  vox::network::NetworkAccess network("http://fake");
  network.setCustomHandler([&](const QString &method,
                               const QString &path,
                               const QByteArray &body,
                               const vox::network::NetworkAccess::HeaderMap &headers) {
    return fake_server.handle(method, path, body, headers);
  });

  vox::network::AuthApi auth_api(network);
  InMemoryAccountsRepository accounts_repo;

  const QString vault_path = QString::fromStdString((temporaryDirectory / "vault.json").string());
  vox::crypto::VaultService vault(vault_path);
  ASSERT_TRUE(vault.unlockWithPassword("password"));

  vox::crypto::IdentityKeyStore identity_store(vault);
  vox::crypto::PrekeyManager prekeys(vault, identity_store);

  vox::services::AuthService auth_service(
      auth_api, network, accounts_repo, vault, identity_store, prekeys, "default", "dev_desktop");

  ASSERT_TRUE(auth_service.registerUser("alice", "derived_secret"));
  ASSERT_TRUE(auth_service.context().has_value());

  vox::services::AuthService restorer(
      auth_api, network, accounts_repo, vault, identity_store, prekeys, "default", "dev_desktop");

  EXPECT_TRUE(restorer.restoreSession());
}

TEST_F(ProjectUnitTestSuite, AuthServiceRejectsEmptyCredentialsNegative) {
  FakeVoxServer fake_server;

  vox::network::NetworkAccess network("http://fake");
  network.setCustomHandler([&](const QString &method,
                               const QString &path,
                               const QByteArray &body,
                               const vox::network::NetworkAccess::HeaderMap &headers) {
    return fake_server.handle(method, path, body, headers);
  });

  vox::network::AuthApi auth_api(network);
  InMemoryAccountsRepository accounts_repo;

  const QString vault_path = QString::fromStdString((temporaryDirectory / "vault2.json").string());
  vox::crypto::VaultService vault(vault_path);
  ASSERT_TRUE(vault.unlockWithPassword("password"));

  vox::crypto::IdentityKeyStore identity_store(vault);
  vox::crypto::PrekeyManager prekeys(vault, identity_store);

  vox::services::AuthService auth_service(
      auth_api, network, accounts_repo, vault, identity_store, prekeys, "default", "dev_desktop");

  EXPECT_FALSE(auth_service.login({}, {}));
  EXPECT_FALSE(auth_service.registerUser({}, "pwd"));
}

TEST_F(ProjectUnitTestSuite, MessageSendServiceQueuesJobOnTransportFailureNegative) {
  ASSERT_TRUE(QSqlDatabase::isDriverAvailable("QSQLITE")) << "QSQLITE driver is unavailable in this Qt build";

  FakeVoxServer fake_server;
  fake_server.setFailNextSend(true);

  vox::network::NetworkAccess network("http://fake");
  network.setCustomHandler([&](const QString &method,
                               const QString &path,
                               const QByteArray &body,
                               const vox::network::NetworkAccess::HeaderMap &headers) {
    return fake_server.handle(method, path, body, headers);
  });
  network.setBearerToken("acc_usr_1");

  vox::storage::Database db;
  ASSERT_TRUE(db.open(QString::fromStdString((temporaryDirectory / "services.sqlite").string())));

  QString migration_error;
  ASSERT_TRUE(vox::storage::Migrations::run(db.database(), &migration_error)) << migration_error.toStdString();

  vox::storage::SqlMessagesRepository messages_repo(db.database());
  vox::storage::SqlJobsRepository jobs_repo(db.database());

  vox::network::ConversationsApi conversations_api(network);
  vox::services::MessageSendService sender(conversations_api, messages_repo, jobs_repo);

  EXPECT_TRUE(sender.sendCiphertext("conv_1", "dev_desktop", "ciphertext"));

  const auto due = jobs_repo.dueJobs(QDateTime::currentMSecsSinceEpoch() + 10'000);
  EXPECT_FALSE(due.isEmpty());
}

TEST_F(ProjectUnitTestSuite, SearchServiceIndexesAndFindsMessagesPositive) {
  ASSERT_TRUE(QSqlDatabase::isDriverAvailable("QSQLITE")) << "QSQLITE driver is unavailable in this Qt build";

  vox::storage::Database db;
  ASSERT_TRUE(db.open(QString::fromStdString((temporaryDirectory / "search.sqlite").string())));

  QString migration_error;
  ASSERT_TRUE(vox::storage::Migrations::run(db.database(), &migration_error)) << migration_error.toStdString();

  vox::storage::SqlSearchRepository search_repo(db.database());
  vox::services::SearchService search_service(search_repo, QByteArray(vox::crypto::kChaCha20KeyBytes, 'k'));

  ASSERT_TRUE(search_service.indexMessage("conv_1", "msg_1", "Hello Vox Search"));

  const auto found = search_service.findMessageIds("vox");
  ASSERT_EQ(found.size(), 1);
  EXPECT_EQ(found.first(), "msg_1");
}
