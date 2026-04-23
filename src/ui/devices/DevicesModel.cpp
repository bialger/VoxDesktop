#include "ui/devices/DevicesModel.hpp"

namespace vox::ui::devices {

DevicesModel::DevicesModel(QObject *parent) : QAbstractListModel(parent) {}

int DevicesModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_devices.size();
}

QVariant DevicesModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_devices.size()) {
        return {};
    }

    const auto &device = m_devices.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return QString("%1 (%2)").arg(device.label, domain::toString(device.trustLevel));
    case DeviceIdRole:
        return device.deviceId;
    case LabelRole:
        return device.label;
    case TrustRole:
        return domain::toString(device.trustLevel);
    case LastSeenRole:
        return device.lastSeen;
    default:
        return {};
    }
}

QHash<int, QByteArray> DevicesModel::roleNames() const {
    return {{DeviceIdRole, "device_id"}, {LabelRole, "label"}, {TrustRole, "trust"}, {LastSeenRole, "last_seen"}};
}

void DevicesModel::setDevices(QVector<domain::Device> devices) {
    beginResetModel();
    m_devices = std::move(devices);
    endResetModel();
}

} // namespace vox::ui::devices
