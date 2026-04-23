#ifndef VOX_UI_CONVERSATIONS_MESSAGEBUBBLEDELEGATE_HPP
#define VOX_UI_CONVERSATIONS_MESSAGEBUBBLEDELEGATE_HPP

#include <QStyledItemDelegate>

namespace vox::ui::conversations {

class MessageBubbleDelegate final : public QStyledItemDelegate {
public:
  explicit MessageBubbleDelegate(QObject *parent = nullptr);

  void setShowAuthors(bool show);

  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
  QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
  bool m_showAuthors{false};
};

} // namespace vox::ui::conversations

#endif // VOX_UI_CONVERSATIONS_MESSAGEBUBBLEDELEGATE_HPP
