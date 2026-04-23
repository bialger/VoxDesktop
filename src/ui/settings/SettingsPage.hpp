#ifndef VOX_UI_SETTINGS_SETTINGSPAGE_HPP
#define VOX_UI_SETTINGS_SETTINGSPAGE_HPP

#include <QWidget>

namespace vox::ui::settings {

class SettingsPage final : public QWidget {
public:
    explicit SettingsPage(QWidget *parent = nullptr);
};

} // namespace vox::ui::settings

#endif // VOX_UI_SETTINGS_SETTINGSPAGE_HPP
