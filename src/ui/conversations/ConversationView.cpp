#include "ui/conversations/ConversationView.hpp"

#include <QVBoxLayout>

#include <memory>

namespace vox::ui::conversations {
namespace {

constexpr int kLayoutMarginPx = 8;

} // namespace

ConversationView::ConversationView(QWidget *parent) :
    QWidget(parent), m_messagesModel(new MessageListModel(this)), m_messagesView(new QListView(this)),
    m_delegate(new MessageBubbleDelegate(m_messagesView)), m_composer(new ComposerWidget(this)) {
  auto layout = std::make_unique<QVBoxLayout>();
  layout->setContentsMargins(kLayoutMarginPx, kLayoutMarginPx, kLayoutMarginPx, kLayoutMarginPx);

  auto title = std::make_unique<QLabel>("No conversation selected", this);
  m_title = title.release();

  m_messagesView->setModel(m_messagesModel);
  m_messagesView->setSelectionMode(QAbstractItemView::NoSelection);
  m_messagesView->setUniformItemSizes(false);
  m_messagesView->setWordWrap(true);

  m_messagesView->setItemDelegate(m_delegate);

  layout->addWidget(m_title);
  layout->addWidget(m_messagesView, 1);
  layout->addWidget(m_composer);

  connect(m_composer, &ComposerWidget::sendRequested, this, &ConversationView::sendMessageRequested);
  setLayout(layout.release());
}

void ConversationView::setConversationTitle(const QString &title) {
  m_title->setText(title);
}

void ConversationView::setShowAuthors(bool show) {
  if (m_delegate != nullptr) {
    m_delegate->setShowAuthors(show);
  }
  if (m_messagesView != nullptr && m_messagesView->model() != nullptr) {
    m_messagesView->viewport()->update();
  }
}

MessageListModel *ConversationView::model() {
  return m_messagesModel;
}

} // namespace vox::ui::conversations
