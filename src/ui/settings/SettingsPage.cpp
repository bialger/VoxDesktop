#include "ui/settings/SettingsPage.hpp"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QVBoxLayout>

namespace vox::ui::settings {

SettingsPage::SettingsPage(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);

  auto *privacyGroup = new QGroupBox("Privacy", this);
  auto *privacyLayout = new QFormLayout(privacyGroup);
  privacyLayout->addRow("Hide notification previews", new QCheckBox(privacyGroup));
  privacyLayout->addRow("Minimize to tray", new QCheckBox(privacyGroup));

  auto *securityGroup = new QGroupBox("Security", this);
  auto *securityLayout = new QFormLayout(securityGroup);
  securityLayout->addRow("Require unlock on start", new QCheckBox(securityGroup));
  securityLayout->addRow("Lock when minimized", new QCheckBox(securityGroup));

  layout->addWidget(privacyGroup);
  layout->addWidget(securityGroup);
  layout->addStretch(1);
}

} // namespace vox::ui::settings
