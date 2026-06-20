#include "ui/shell/AppStack.hpp"

#include <QHBoxLayout>
#include <QSplitter>
#include <QTabWidget>

#include <memory>

namespace vox::ui::shell {
namespace {

constexpr int kConversationListStretchFactor = 2;
constexpr int kConversationViewStretchFactor = 5;
constexpr int kRightTabsStretchFactor = 3;

} // namespace

AppStack::AppStack(QWidget *parent) :
    QStackedWidget(parent), m_welcomePage(new auth::WelcomePage(this)), m_loginPage(new auth::LoginPage(this)),
    m_registerPage(new auth::RegisterPage(this)), m_mainPage(new QWidget(this)) {
  addWidget(m_welcomePage);
  addWidget(m_loginPage);
  addWidget(m_registerPage);

  auto main_layout = std::make_unique<QHBoxLayout>();
  main_layout->setContentsMargins(0, 0, 0, 0);

  auto splitter = std::make_unique<QSplitter>(Qt::Horizontal, m_mainPage);

  auto conversation_list = std::make_unique<conversations::ConversationListPane>(splitter.get());
  m_conversationListPane = conversation_list.release();
  auto conversation_view = std::make_unique<conversations::ConversationView>(splitter.get());
  m_conversationView = conversation_view.release();

  auto right_tabs = std::make_unique<QTabWidget>(splitter.get());
  auto settings_page = std::make_unique<settings::SettingsPage>(right_tabs.get());
  m_settingsPage = settings_page.release();

  right_tabs->addTab(m_settingsPage, "Settings");

  splitter->addWidget(m_conversationListPane);
  splitter->addWidget(m_conversationView);
  splitter->addWidget(right_tabs.release());

  splitter->setStretchFactor(0, kConversationListStretchFactor);
  splitter->setStretchFactor(1, kConversationViewStretchFactor);
  splitter->setStretchFactor(2, kRightTabsStretchFactor);

  main_layout->addWidget(splitter.release());
  m_mainPage->setLayout(main_layout.release());

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
