#ifndef VOX_UI_DEVICES_DEVICESPAGE_HPP
#define VOX_UI_DEVICES_DEVICESPAGE_HPP

#include <QListView>
#include <QWidget>

#include "ui/devices/DevicesModel.hpp"

namespace vox::ui::devices {

class DevicesPage final : public QWidget {
public:
    explicit DevicesPage(QWidget *parent = nullptr);

    DevicesModel *model();

private:
    DevicesModel *m_model{nullptr};
    QListView *m_listView{nullptr};
};

} // namespace vox::ui::devices

#endif // VOX_UI_DEVICES_DEVICESPAGE_HPP
