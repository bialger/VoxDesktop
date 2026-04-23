#ifndef VOX_UI_DEVICES_DEVICESMODEL_HPP
#define VOX_UI_DEVICES_DEVICESMODEL_HPP

#include <QAbstractListModel>

#include "domain/Models.hpp"

namespace vox::ui::devices {

class DevicesModel final : public QAbstractListModel {
public:
    enum Role { DeviceIdRole = Qt::UserRole + 1, LabelRole, TrustRole, LastSeenRole };

    explicit DevicesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setDevices(QVector<domain::Device> devices);

private:
    QVector<domain::Device> m_devices;
};

} // namespace vox::ui::devices

#endif // VOX_UI_DEVICES_DEVICESMODEL_HPP
