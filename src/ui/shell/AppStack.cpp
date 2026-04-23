#include "ui/shell/AppStack.hpp"

#include <QHBoxLayout>
#include <QSplitter>
#include <QTabWidget>

namespace vox::ui::shell {

AppStack::AppStack(QWidget *parent) : QStackedWidget(parent) {
  m_welcomePage = new auth::WelcomePage(this);
  m_loginPage = new auth::LoginPage(this);
  m_registerPage = new auth::RegisterPage(this);

  addWidget(m_welcomePage);
  addWidget(m_loginPage);
  addWidget(m_registerPage);

  m_mainPage = new QWidget(this);
  auto *mainLayout = new QHBoxLayout(m_mainPage);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  auto *splitter = new QSplitter(Qt::Horizontal, m_mainPage);

  m_conversationListPane = new conversations::ConversationListPane(splitter);
  m_conversationView = new conversations::ConversationView(splitter);

  auto *rightTabs = new QTabWidget(splitter);
  m_settingsPage = new settings::SettingsPage(rightTabs);

  rightTabs->addTab(m_settingsPage, "Settings");

  splitter->addWidget(m_conversationListPane);
  splitter->addWidget(m_conversationView);
  splitter->addWidget(rightTabs);

  splitter->setStretchFactor(0, 2);
  splitter->setStretchFactor(1, 5);
  splitter->setStretchFactor(2, 3);

  mainLayout->addWidget(splitter);

  addWidget(m_mainPage);
  showWelcome();
}

void AppStack::showWelcome() {
  setCurrentWidget(m_welcomePage);
}

void AppStack::showLogin() {
  setCurrentWidget(m_loginPage);
}

void AppStack::showRegister() {
  setCurrentWidget(m_registerPage);
}

void AppStack::showMain() {
  setCurrentWidget(m_mainPage);
}

auth::WelcomePage *AppStack::welcomePage() const {
  return m_welcomePage;
}

auth::LoginPage *AppStack::loginPage() const {
  return m_loginPage;
}

auth::RegisterPage *AppStack::registerPage() const {
  return m_registerPage;
}

conversations::ConversationListPane *AppStack::conversationListPane() const {
  return m_conversationListPane;
}

conversations::ConversationView *AppStack::conversationView() const {
  return m_conversationView;
}

settings::SettingsPage *AppStack::settingsPage() const {
  return m_settingsPage;
}

} // namespace vox::ui::shell
