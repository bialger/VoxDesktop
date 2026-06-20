#include "ui/conversations/MessageBubbleDelegate.hpp"

#include "ui/conversations/MessageListModel.hpp"

#include <QDateTime>
#include <QPainter>

namespace vox::ui::conversations {
namespace {

constexpr int kOutgoingBubbleRed = 46;
constexpr int kOutgoingBubbleGreen = 125;
constexpr int kOutgoingBubbleBlue = 50;
constexpr int kIncomingBubbleRed = 40;
constexpr int kIncomingBubbleGreen = 53;
constexpr int kIncomingBubbleBlue = 147;
constexpr int kTextColorChannel = 255;
constexpr int kStampColorChannel = 230;
constexpr int kBubbleMarginPx = 8;
constexpr int kBubblePaddingPx = 10;
constexpr int kBubbleCornerRadiusPx = 10;
constexpr double kBubbleMaxWidthRatio = 0.70;
constexpr int kBodyFontMinPointSize = 9;
constexpr int kMetaFontMinPointSize = 8;
constexpr int kTextMeasureHeightPx = 10'000;

QColor BubbleColor(bool outgoing) {
  return outgoing ? QColor(kOutgoingBubbleRed, kOutgoingBubbleGreen, kOutgoingBubbleBlue)
                  : QColor(kIncomingBubbleRed, kIncomingBubbleGreen, kIncomingBubbleBlue);
}

QColor TextColor() {
  return {kTextColorChannel, kTextColorChannel, kTextColorChannel};
}

} // namespace

MessageBubbleDelegate::MessageBubbleDelegate(QObject *parent) : QStyledItemDelegate(parent) {
}

void MessageBubbleDelegate::setShowAuthors(bool show) {
  m_showAuthors = show;
}

void MessageBubbleDelegate::paint(QPainter *painter,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const {
  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);

  const bool outgoing = index.data(MessageListModel::OutgoingRole).toBool();
  const QString body = index.data(MessageListModel::BodyRole).toString();
  const QString author = index.data(MessageListModel::AuthorRole).toString();
  const QDateTime ts = index.data(MessageListModel::TimestampRole).toDateTime().toLocalTime();
  const QString stamp = ts.toString("HH:mm");

  const int margin = kBubbleMarginPx;
  const int padding = kBubblePaddingPx;
  const int radius = kBubbleCornerRadiusPx;
  const int max_width = static_cast<int>(option.rect.width() * kBubbleMaxWidthRatio);

  QFont body_font = option.font;
  body_font.setPointSize(std::max(kBodyFontMinPointSize, body_font.pointSize()));

  QFont meta_font = option.font;
  meta_font.setPointSize(std::max(kMetaFontMinPointSize, meta_font.pointSize() - 1));

  QFontMetrics body_fm(body_font);
  QFontMetrics meta_fm(meta_font);

  QString header;
  if (m_showAuthors && !outgoing && !author.isEmpty()) {
    header = author;
  }

  const QRect text_rect0(0, 0, max_width, kTextMeasureHeightPx);
  const QRect header_rect = header.isEmpty() ? QRect() : meta_fm.boundingRect(text_rect0, Qt::TextWordWrap, header);
  const QRect body_rect = body_fm.boundingRect(text_rect0, Qt::TextWordWrap, body);
  const QRect stamp_rect = meta_fm.boundingRect(text_rect0, Qt::TextSingleLine, stamp);

  int content_w = std::max({body_rect.width(), header_rect.width(), stamp_rect.width()});
  int content_h = body_rect.height() + stamp_rect.height();
  if (!header.isEmpty()) {
    content_h += header_rect.height();
  }

  const int bubble_w = content_w + padding * 2;
  const int bubble_h = content_h + padding * 2;

  int x = option.rect.x() + margin;
  if (outgoing) {
    x = option.rect.right() - margin - bubble_w;
  }
  const int y = option.rect.y() + margin;

  QRect bubble_rect(x, y, bubble_w, bubble_h);

  // background
  painter->setPen(Qt::NoPen);
  painter->setBrush(BubbleColor(outgoing));
  painter->drawRoundedRect(bubble_rect, radius, radius);

  // text
  painter->setPen(TextColor());
  int cursor_y = bubble_rect.y() + padding;
  const int text_x = bubble_rect.x() + padding;
  const int text_w = bubble_rect.width() - padding * 2;

  painter->setFont(meta_font);
  if (!header.isEmpty()) {
    painter->drawText(QRect(text_x, cursor_y, text_w, header_rect.height()),
                      Qt::TextSingleLine | Qt::AlignLeft | Qt::AlignTop,
                      header);
    cursor_y += header_rect.height();
  }

  painter->setFont(body_font);
  painter->drawText(
      QRect(text_x, cursor_y, text_w, body_rect.height()), Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, body);
  cursor_y += body_rect.height();

  painter->setFont(meta_font);
  painter->setPen(QColor(kStampColorChannel, kStampColorChannel, kStampColorChannel));
  painter->drawText(QRect(text_x, cursor_y, text_w, stamp_rect.height()),
                    Qt::TextSingleLine | Qt::AlignRight | Qt::AlignBottom,
                    stamp);

  painter->restore();
}

QSize MessageBubbleDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
  const bool outgoing = index.data(MessageListModel::OutgoingRole).toBool();
  const QString body = index.data(MessageListModel::BodyRole).toString();
  const QString author = index.data(MessageListModel::AuthorRole).toString();
  const QDateTime ts = index.data(MessageListModel::TimestampRole).toDateTime().toLocalTime();
  const QString stamp = ts.toString("HH:mm");

  const int margin = kBubbleMarginPx;
  const int padding = kBubblePaddingPx;
  const int max_width = static_cast<int>(option.rect.width() * kBubbleMaxWidthRatio);

  QFont body_font = option.font;
  body_font.setPointSize(std::max(kBodyFontMinPointSize, body_font.pointSize()));

  QFont meta_font = option.font;
  meta_font.setPointSize(std::max(kMetaFontMinPointSize, meta_font.pointSize() - 1));

  QFontMetrics body_fm(body_font);
  QFontMetrics meta_fm(meta_font);

  QString header;
  if (m_showAuthors && !outgoing && !author.isEmpty()) {
    header = author;
  }

  const QRect text_rect0(0, 0, max_width, kTextMeasureHeightPx);
  const QRect header_rect = header.isEmpty() ? QRect() : meta_fm.boundingRect(text_rect0, Qt::TextWordWrap, header);
  const QRect body_rect = body_fm.boundingRect(text_rect0, Qt::TextWordWrap, body);
  const QRect stamp_rect = meta_fm.boundingRect(text_rect0, Qt::TextSingleLine, stamp);

  int content_w = std::max({body_rect.width(), header_rect.width(), stamp_rect.width()});
  int content_h = body_rect.height() + stamp_rect.height();
  if (!header.isEmpty()) {
    content_h += header_rect.height();
  }

  const int bubble_w = content_w + padding * 2;
  const int bubble_h = content_h + padding * 2;

  return {bubble_w + margin * 2, bubble_h + margin * 2};
}

} // namespace vox::ui::conversations
