#ifndef VOX_UI_SHELL_APPSTACK_HPP
#define VOX_UI_SHELL_APPSTACK_HPP

#include <QStackedWidget>

#include "ui/auth/LoginPage.hpp"
#include "ui/auth/RegisterPage.hpp"
#include "ui/auth/WelcomePage.hpp"
#include "ui/conversations/ConversationListPane.hpp"
#include "ui/conversations/ConversationView.hpp"
#include "ui/settings/SettingsPage.hpp"

namespace vox::ui::shell {

class AppStack final : public QStackedWidget {
public:
  explicit AppStack(QWidget *parent = nullptr);

  void showWelcome();
  void showLogin();
  void showRegister();
  void showMain();

  auth::WelcomePage *welcomePage() const;
  auth::LoginPage *loginPage() const;
  auth::RegisterPage *registerPage() const;

  conversations::ConversationListPane *conversationListPane() const;
  conversations::ConversationView *conversationView() const;
  settings::SettingsPage *settingsPage() const;

private:
  QWidget *m_mainPage{nullptr};
  auth::WelcomePage *m_welcomePage{nullptr};
  auth::LoginPage *m_loginPage{nullptr};
  auth::RegisterPage *m_registerPage{nullptr};

  conversations::ConversationListPane *m_conversationListPane{nullptr};
  conversations::ConversationView *m_conversationView{nullptr};
  settings::SettingsPage *m_settingsPage{nullptr};
};

} // namespace vox::ui::shell

#endif // VOX_UI_SHELL_APPSTACK_HPP
