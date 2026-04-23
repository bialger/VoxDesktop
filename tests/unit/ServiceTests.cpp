#include <QDateTime>
#include <QSqlDatabase>
#include <gtest/gtest.h>

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
    m_account = account;
    return true;
  }

  std::optional<vox::domain::Account> activeAccount() const override {
    return m_account;
  }

  bool deleteAccount(const QString &accountId) override {
    if (!m_account.has_value()) {
      return true;
    }
    if (m_account->accountId != accountId) {
      return false;
    }
    m_account.reset();
    return true;
  }

private:
  std::optional<vox::domain::Account> m_account;
};

} // namespace

TEST_F(ProjectUnitTestSuite, AuthServiceRegisterAndRestoreSessionPositive) {
  FakeVoxServer fakeServer;

  vox::network::NetworkAccess network("http://fake");
  network.setCustomHandler([&](const QString &method,
                               const QString &path,
                               const QByteArray &body,
                               const vox::network::NetworkAccess::HeaderMap &headers) {
    return fakeServer.handle(method, path, body, headers);
  });

  vox::network::AuthApi authApi(network);
  InMemoryAccountsRepository accountsRepo;

  const QString vaultPath = QString::fromStdString((temporaryDirectory / "vault.json").string());
  vox::crypto::VaultService vault(vaultPath);
  ASSERT_TRUE(vault.unlockWithPassword("password"));

  vox::crypto::IdentityKeyStore identityStore(vault);
  vox::crypto::PrekeyManager prekeys(vault, identityStore);

  vox::services::AuthService authService(
      authApi, network, accountsRepo, vault, identityStore, prekeys, "default", "dev_desktop");

  ASSERT_TRUE(authService.registerUser("alice", "derived_secret"));
  ASSERT_TRUE(authService.context().has_value());

  vox::services::AuthService restorer(
      authApi, network, accountsRepo, vault, identityStore, prekeys, "default", "dev_desktop");

  EXPECT_TRUE(restorer.restoreSession());
}

TEST_F(ProjectUnitTestSuite, AuthServiceRejectsEmptyCredentialsNegative) {
  FakeVoxServer fakeServer;

  vox::network::NetworkAccess network("http://fake");
  network.setCustomHandler([&](const QString &method,
                               const QString &path,
                               const QByteArray &body,
                               const vox::network::NetworkAccess::HeaderMap &headers) {
    return fakeServer.handle(method, path, body, headers);
  });

  vox::network::AuthApi authApi(network);
  InMemoryAccountsRepository accountsRepo;

  const QString vaultPath = QString::fromStdString((temporaryDirectory / "vault2.json").string());
  vox::crypto::VaultService vault(vaultPath);
  ASSERT_TRUE(vault.unlockWithPassword("password"));

  vox::crypto::IdentityKeyStore identityStore(vault);
  vox::crypto::PrekeyManager prekeys(vault, identityStore);

  vox::services::AuthService authService(
      authApi, network, accountsRepo, vault, identityStore, prekeys, "default", "dev_desktop");

  EXPECT_FALSE(authService.login({}, {}));
  EXPECT_FALSE(authService.registerUser({}, "pwd"));
}

TEST_F(ProjectUnitTestSuite, MessageSendServiceQueuesJobOnTransportFailureNegative) {
  ASSERT_TRUE(QSqlDatabase::isDriverAvailable("QSQLITE")) << "QSQLITE driver is unavailable in this Qt build";

  FakeVoxServer fakeServer;
  fakeServer.setFailNextSend(true);

  vox::network::NetworkAccess network("http://fake");
  network.setCustomHandler([&](const QString &method,
                               const QString &path,
                               const QByteArray &body,
                               const vox::network::NetworkAccess::HeaderMap &headers) {
    return fakeServer.handle(method, path, body, headers);
  });
  network.setBearerToken("acc_usr_1");

  vox::storage::Database db;
  ASSERT_TRUE(db.open(QString::fromStdString((temporaryDirectory / "services.sqlite").string())));

  QString migrationError;
  ASSERT_TRUE(vox::storage::Migrations::run(db.database(), &migrationError)) << migrationError.toStdString();

  vox::storage::SqlMessagesRepository messagesRepo(db.database());
  vox::storage::SqlJobsRepository jobsRepo(db.database());

  vox::network::ConversationsApi conversationsApi(network);
  vox::services::MessageSendService sender(conversationsApi, messagesRepo, jobsRepo);

  EXPECT_TRUE(sender.sendCiphertext("conv_1", "dev_desktop", "ciphertext"));

  const auto due = jobsRepo.dueJobs(QDateTime::currentMSecsSinceEpoch() + 10'000);
  EXPECT_FALSE(due.isEmpty());
}

TEST_F(ProjectUnitTestSuite, SearchServiceIndexesAndFindsMessagesPositive) {
  ASSERT_TRUE(QSqlDatabase::isDriverAvailable("QSQLITE")) << "QSQLITE driver is unavailable in this Qt build";

  vox::storage::Database db;
  ASSERT_TRUE(db.open(QString::fromStdString((temporaryDirectory / "search.sqlite").string())));

  QString migrationError;
  ASSERT_TRUE(vox::storage::Migrations::run(db.database(), &migrationError)) << migrationError.toStdString();

  vox::storage::SqlSearchRepository searchRepo(db.database());
  vox::services::SearchService searchService(searchRepo, QByteArray(32, 'k'));

  ASSERT_TRUE(searchService.indexMessage("conv_1", "msg_1", "Hello Vox Search"));

  const auto found = searchService.findMessageIds("vox");
  ASSERT_EQ(found.size(), 1);
  EXPECT_EQ(found.first(), "msg_1");
}
