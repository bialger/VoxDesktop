#ifndef VOX_STORAGE_MIGRATIONS_HPP
#define VOX_STORAGE_MIGRATIONS_HPP

#include <QSqlDatabase>
#include <QString>

namespace vox::storage {

class Migrations final {
public:
    static bool run(QSqlDatabase db, QString *error = nullptr);
    static int currentVersion();
};

} // namespace vox::storage

#endif // VOX_STORAGE_MIGRATIONS_HPP
