#ifndef VOX_STORAGE_SQLHELPERS_HPP
#define VOX_STORAGE_SQLHELPERS_HPP

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

namespace vox::storage::SqlHelpers {

bool prepareAndBind(QSqlQuery &query, const QString &statement, const QList<QVariant> &bindValues);
bool tableExists(const QSqlDatabase &db, const QString &tableName);

} // namespace vox::storage::SqlHelpers

#endif // VOX_STORAGE_SQLHELPERS_HPP
