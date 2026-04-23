#ifndef VOX_UI_CONVERSATIONS_CONVERSATIONVIEW_HPP
#define VOX_UI_CONVERSATIONS_CONVERSATIONVIEW_HPP

#include <QLabel>
#include <QListView>
#include <QWidget>

#include "ui/conversations/ComposerWidget.hpp"
#include "ui/conversations/MessageBubbleDelegate.hpp"
#include "ui/conversations/MessageListModel.hpp"

namespace vox::ui::conversations {

class ConversationView final : public QWidget {
  Q_OBJECT

public:
  explicit ConversationView(QWidget *parent = nullptr);

  void setConversationTitle(const QString &title);
  void setShowAuthors(bool show);
  MessageListModel *model();

signals:
  void sendMessageRequested(const QString &text);

private:
  QLabel *m_title{nullptr};
  QListView *m_messagesView{nullptr};
  MessageListModel *m_messagesModel{nullptr};
  ComposerWidget *m_composer{nullptr};
  MessageBubbleDelegate *m_delegate{nullptr};
};

} // namespace vox::ui::conversations

#endif // VOX_UI_CONVERSATIONS_CONVERSATIONVIEW_HPP
