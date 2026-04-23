#ifndef VOX_STORAGE_DATABASE_HPP
#define VOX_STORAGE_DATABASE_HPP

#include <QSqlDatabase>
#include <QString>

namespace vox::storage {

class Database final {
public:
    Database();
    ~Database();

    bool open(const QString &databasePath);
    void close();

    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] bool isOnOwnerThread() const;

    [[nodiscard]] QSqlDatabase database() const;
    [[nodiscard]] QString lastError() const;

private:
    QString m_connectionName;
    QSqlDatabase m_database;
    QString m_lastError;
    Qt::HANDLE m_ownerThread{nullptr};
};

} // namespace vox::storage

#endif // VOX_STORAGE_DATABASE_HPP
