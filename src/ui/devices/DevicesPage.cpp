#include "ui/devices/DevicesPage.hpp"

#include <QVBoxLayout>

namespace vox::ui::devices {

DevicesPage::DevicesPage(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);

  m_model = new DevicesModel(this);
  m_listView = new QListView(this);
  m_listView->setModel(m_model);

  layout->addWidget(m_listView);
}

DevicesModel *DevicesPage::model() {
  return m_model;
}

} // namespace vox::ui::devices
