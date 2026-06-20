#include "ui/devices/DevicesPage.hpp"

#include <QVBoxLayout>

#include <memory>

namespace vox::ui::devices {

DevicesPage::DevicesPage(QWidget *parent) :
    QWidget(parent), m_model(new DevicesModel(this)), m_listView(new QListView(this)) {
  auto layout = std::make_unique<QVBoxLayout>();

  m_listView->setModel(m_model);

  layout->addWidget(m_listView);
  setLayout(layout.release());
}

DevicesModel *DevicesPage::model() {
  return m_model;
}

} // namespace vox::ui::devices
