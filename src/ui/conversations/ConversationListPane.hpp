#ifndef VOX_UI_CONVERSATIONS_CONVERSATIONLISTPANE_HPP
#define VOX_UI_CONVERSATIONS_CONVERSATIONLISTPANE_HPP

#include <QLineEdit>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QTabBar>
#include <QToolButton>
#include <QWidget>

#include "ui/conversations/ConversationListModel.hpp"

namespace vox::ui::conversations {

class ConversationListPane final : public QWidget {
  Q_OBJECT

public:
  explicit ConversationListPane(QWidget *parent = nullptr);

  ConversationListModel *model();

signals:
  void conversationSelected(const QString &conversationId);
  void createDmRequested(const QString &username);
  void subscribeChannelRequested(const QString &conversationIdOrText);
  void createGroupRequested(const QStringList &usernames);
  void createChannelRequested(const QStringList &adminUsernames);

private:
  ConversationListModel *m_model{nullptr};
  QSortFilterProxyModel *m_proxyModel{nullptr};
  QLineEdit *m_searchEdit{nullptr};
  QListView *m_listView{nullptr};
  QTabBar *m_tabBar{nullptr};
  QToolButton *m_addButton{nullptr};
};

} // namespace vox::ui::conversations

#endif // VOX_UI_CONVERSATIONS_CONVERSATIONLISTPANE_HPP
