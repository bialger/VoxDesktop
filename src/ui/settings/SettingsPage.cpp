#include "ui/settings/SettingsPage.hpp"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QVBoxLayout>

#include <memory>

namespace vox::ui::settings {

SettingsPage::SettingsPage(QWidget *parent) : QWidget(parent) {
  auto layout = std::make_unique<QVBoxLayout>();

  auto *privacy_group = new QGroupBox("Privacy", this);
  auto privacy_layout = std::make_unique<QFormLayout>();
  auto hide_previews = std::make_unique<QCheckBox>(privacy_group);
  auto minimize_to_tray = std::make_unique<QCheckBox>(privacy_group);
  privacy_layout->addRow("Hide notification previews", hide_previews.release());
  privacy_layout->addRow("Minimize to tray", minimize_to_tray.release());
  privacy_group->setLayout(privacy_layout.release());

  auto *security_group = new QGroupBox("Security", this);
  auto security_layout = std::make_unique<QFormLayout>();
  auto require_unlock = std::make_unique<QCheckBox>(security_group);
  auto lock_when_minimized = std::make_unique<QCheckBox>(security_group);
  security_layout->addRow("Require unlock on start", require_unlock.release());
  security_layout->addRow("Lock when minimized", lock_when_minimized.release());
  security_group->setLayout(security_layout.release());

  layout->addWidget(privacy_group);
  layout->addWidget(security_group);
  layout->addStretch(1);
  setLayout(layout.release());
}

} // namespace vox::ui::settings
