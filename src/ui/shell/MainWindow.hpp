#ifndef VOX_UI_SHELL_MAINWINDOW_HPP
#define VOX_UI_SHELL_MAINWINDOW_HPP

#include <QMainWindow>

#include "domain/Models.hpp"
#include "ui/shell/AppStack.hpp"

namespace vox::ui::shell {

class MainWindow final : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);

  void setConversations(QVector<domain::Conversation> conversations);
  void setMessages(QVector<domain::Message> messages);
  void setConversationTitle(const QString &title);
  void setShowMessageAuthors(bool show);

  void showWelcome();
  void showMain();

signals:
  void serverBaseUrlReady(const QString &baseUrl);
  void logoutRequested();
  void loginSubmitted(const QString &username, const QString &passwordDerived);
  void registerSubmitted(const QString &username, const QString &passwordDerived);
  void sendMessageSubmitted(const QString &text);
  void conversationSelected(const QString &conversationId);
  void createDmRequested(const QString &username);
  void subscribeChannelRequested(const QString &conversationId);
  void createGroupRequested(const QStringList &usernames);
  void createChannelRequested(const QStringList &adminUsernames);

private:
  AppStack *m_stack{nullptr};
};

} // namespace vox::ui::shell

#endif // VOX_UI_SHELL_MAINWINDOW_HPP
