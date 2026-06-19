#include "storage/SqlHelpers.hpp"

namespace vox::storage::SqlHelpers {

bool prepareAndBind(QSqlQuery &query, const QString &statement, const QList<QVariant> &bindValues) {
  if (!query.prepare(statement)) {
    return false;
  }

  for (const auto &value : bindValues) {
    query.addBindValue(value);
  }
  return true;
}

bool tableExists(const QSqlDatabase &db, const QString &tableName) {
  return db.tables().contains(tableName);
}

} // namespace vox::storage::SqlHelpers
