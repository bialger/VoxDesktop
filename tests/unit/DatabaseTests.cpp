#include <gtest/gtest.h>

#include <QSqlDatabase>
#include <QStringList>

#include "storage/Database.hpp"
#include "storage/Migrations.hpp"
#include "tests/test_suites/ProjectUnitTestSuite.hpp"

TEST_F(ProjectUnitTestSuite, DatabaseOpenAndMigrateCreatesRequiredTables) {
  ASSERT_TRUE(QSqlDatabase::isDriverAvailable("QSQLITE")) << "QSQLITE driver is unavailable in this Qt build";

  vox::storage::Database db;
  const auto path = (temporaryDirectory / "vox.sqlite").string();

  ASSERT_TRUE(db.open(QString::fromStdString(path)));

  QString error;
  ASSERT_TRUE(vox::storage::Migrations::run(db.database(), &error)) << error.toStdString();

  const auto tables = db.database().tables();
  EXPECT_TRUE(tables.contains("accounts"));
  EXPECT_TRUE(tables.contains("messages"));
  EXPECT_TRUE(tables.contains("sync_records"));
  EXPECT_TRUE(tables.contains("jobs"));
}

TEST_F(ProjectUnitTestSuite, MigrationsAreIdempotent) {
  ASSERT_TRUE(QSqlDatabase::isDriverAvailable("QSQLITE")) << "QSQLITE driver is unavailable in this Qt build";

  vox::storage::Database db;
  const auto path = (temporaryDirectory / "idempotent.sqlite").string();

  ASSERT_TRUE(db.open(QString::fromStdString(path)));

  QString error;
  ASSERT_TRUE(vox::storage::Migrations::run(db.database(), &error)) << error.toStdString();
  ASSERT_TRUE(vox::storage::Migrations::run(db.database(), &error)) << error.toStdString();

  EXPECT_EQ(vox::storage::Migrations::currentVersion(), 1);
}

TEST_F(ProjectUnitTestSuite, DatabaseOpenFailsOnInvalidPath) {
  ASSERT_TRUE(QSqlDatabase::isDriverAvailable("QSQLITE")) << "QSQLITE driver is unavailable in this Qt build";

  vox::storage::Database db;

  const QString invalidPath =
      QString::fromStdString((temporaryDirectory / "missing" / "nested" / "db.sqlite").string());
  EXPECT_FALSE(db.open(invalidPath));
  EXPECT_FALSE(db.lastError().isEmpty());
}
