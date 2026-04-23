#include "storage/Database.hpp"

#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QUuid>

namespace vox::storage {

Database::Database() = default;

Database::~Database() {
  close();
}

bool Database::open(const QString &databasePath) {
  if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
    m_lastError = "QSQLITE driver is not available";
    return false;
  }

  close();
  m_connectionName = QString("vox_db_%1").arg(QUuid::createUuid().toString(QUuid::Id128));
  m_database = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
  m_database.setDatabaseName(databasePath);

  if (!m_database.open()) {
    m_lastError = m_database.lastError().text();
    return false;
  }

  m_ownerThread = QThread::currentThreadId();
  m_lastError.clear();

  QSqlQuery pragma(m_database);
  pragma.exec("PRAGMA foreign_keys = ON");
  pragma.exec("PRAGMA journal_mode = WAL");

  return true;
}

void Database::close() {
  if (m_database.isValid()) {
    m_database.close();
  }
  const QString connectionName = m_connectionName;
  m_database = {};
  if (!connectionName.isEmpty() && QSqlDatabase::contains(connectionName)) {
    QSqlDatabase::removeDatabase(connectionName);
  }
  m_connectionName.clear();
}

bool Database::isOpen() const {
  return m_database.isOpen();
}

bool Database::isOnOwnerThread() const {
  return m_ownerThread == QThread::currentThreadId();
}

QSqlDatabase Database::database() const {
  return m_database;
}

QString Database::lastError() const {
  return m_lastError;
}

} // namespace vox::storage
