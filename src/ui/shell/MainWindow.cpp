#include "ui/shell/MainWindow.hpp"

#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>

namespace vox::ui::shell {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Vox Desktop");
    resize(1360, 840);

    m_stack = new AppStack(this);
    setCentralWidget(m_stack);

    auto *sessionMenu = menuBar()->addMenu("Session");
    auto *showWelcomeAction = sessionMenu->addAction("Show Welcome");
    auto *showMainAction = sessionMenu->addAction("Show Main");
    sessionMenu->addSeparator();
    auto *logoutAction = sessionMenu->addAction("Logout");

    connect(showWelcomeAction, &QAction::triggered, m_stack, &AppStack::showWelcome);
    connect(showMainAction, &QAction::triggered, m_stack, &AppStack::showMain);
    connect(logoutAction, &QAction::triggered, this, &MainWindow::logoutRequested);

    connect(m_stack->welcomePage(), &auth::WelcomePage::serverBaseUrlReady, this, &MainWindow::serverBaseUrlReady);
    connect(m_stack->welcomePage(), &auth::WelcomePage::loginRequested, m_stack, &AppStack::showLogin);
    connect(m_stack->welcomePage(), &auth::WelcomePage::registerRequested, m_stack, &AppStack::showRegister);

    connect(m_stack->loginPage(), &auth::LoginPage::backRequested, m_stack, &AppStack::showWelcome);
    connect(m_stack->registerPage(), &auth::RegisterPage::backRequested, m_stack, &AppStack::showWelcome);

    connect(m_stack->loginPage(), &auth::LoginPage::submitLogin, this, &MainWindow::loginSubmitted);
    connect(m_stack->registerPage(), &auth::RegisterPage::submitRegister, this, &MainWindow::registerSubmitted);

    connect(m_stack->conversationView(),
            &conversations::ConversationView::sendMessageRequested,
            this,
            &MainWindow::sendMessageSubmitted);
    connect(m_stack->conversationListPane(),
            &conversations::ConversationListPane::conversationSelected,
            this,
            &MainWindow::conversationSelected);

    connect(m_stack->conversationListPane(),
            &conversations::ConversationListPane::createDmRequested,
            this,
            &MainWindow::createDmRequested);
    connect(m_stack->conversationListPane(),
            &conversations::ConversationListPane::subscribeChannelRequested,
            this,
            &MainWindow::subscribeChannelRequested);
    connect(m_stack->conversationListPane(),
            &conversations::ConversationListPane::createGroupRequested,
            this,
            &MainWindow::createGroupRequested);
    connect(m_stack->conversationListPane(),
            &conversations::ConversationListPane::createChannelRequested,
            this,
            &MainWindow::createChannelRequested);

    statusBar()->showMessage("Ready");
}

void MainWindow::setConversations(QVector<domain::Conversation> conversations) {
    m_stack->conversationListPane()->model()->setConversations(std::move(conversations));
}

void MainWindow::setMessages(QVector<domain::Message> messages) {
    m_stack->conversationView()->model()->setMessages(std::move(messages));
}

void MainWindow::setConversationTitle(const QString &title) {
    m_stack->conversationView()->setConversationTitle(title);
}

void MainWindow::setShowMessageAuthors(bool show) {
    m_stack->conversationView()->setShowAuthors(show);
}

void MainWindow::showWelcome() {
    m_stack->showWelcome();
}

void MainWindow::showMain() {
    m_stack->showMain();
}

} // namespace vox::ui::shell
