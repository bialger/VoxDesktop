#include "ui/conversations/MessageBubbleDelegate.hpp"

#include "ui/conversations/MessageListModel.hpp"

#include <QDateTime>
#include <QPainter>

namespace vox::ui::conversations {
namespace {

QColor bubbleColor(bool outgoing) {
    return outgoing ? QColor(46, 125, 50) : QColor(40, 53, 147); // green / indigo-ish
}

QColor textColor() {
    return QColor(255, 255, 255);
}

} // namespace

MessageBubbleDelegate::MessageBubbleDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

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

    const int margin = 8;
    const int padding = 10;
    const int radius = 10;
    const int maxWidth = static_cast<int>(option.rect.width() * 0.70);

    QFont bodyFont = option.font;
    bodyFont.setPointSize(std::max(9, bodyFont.pointSize()));

    QFont metaFont = option.font;
    metaFont.setPointSize(std::max(8, metaFont.pointSize() - 1));

    QFontMetrics bodyFm(bodyFont);
    QFontMetrics metaFm(metaFont);

    QString header;
    if (m_showAuthors && !outgoing && !author.isEmpty()) {
        header = author;
    }

    const QRect textRect0(0, 0, maxWidth, 10'000);
    const QRect headerRect = header.isEmpty() ? QRect() : metaFm.boundingRect(textRect0, Qt::TextWordWrap, header);
    const QRect bodyRect = bodyFm.boundingRect(textRect0, Qt::TextWordWrap, body);
    const QRect stampRect = metaFm.boundingRect(textRect0, Qt::TextSingleLine, stamp);

    int contentW = std::max({bodyRect.width(), headerRect.width(), stampRect.width()});
    int contentH = bodyRect.height() + stampRect.height();
    if (!header.isEmpty()) {
        contentH += headerRect.height();
    }

    const int bubbleW = contentW + padding * 2;
    const int bubbleH = contentH + padding * 2;

    int x = option.rect.x() + margin;
    if (outgoing) {
        x = option.rect.right() - margin - bubbleW;
    }
    const int y = option.rect.y() + margin;

    QRect bubbleRect(x, y, bubbleW, bubbleH);

    // background
    painter->setPen(Qt::NoPen);
    painter->setBrush(bubbleColor(outgoing));
    painter->drawRoundedRect(bubbleRect, radius, radius);

    // text
    painter->setPen(textColor());
    int cursorY = bubbleRect.y() + padding;
    const int textX = bubbleRect.x() + padding;
    const int textW = bubbleRect.width() - padding * 2;

    painter->setFont(metaFont);
    if (!header.isEmpty()) {
        painter->drawText(QRect(textX, cursorY, textW, headerRect.height()),
                          Qt::TextSingleLine | Qt::AlignLeft | Qt::AlignTop,
                          header);
        cursorY += headerRect.height();
    }

    painter->setFont(bodyFont);
    painter->drawText(QRect(textX, cursorY, textW, bodyRect.height()),
                      Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                      body);
    cursorY += bodyRect.height();

    painter->setFont(metaFont);
    painter->setPen(QColor(230, 230, 230));
    painter->drawText(QRect(textX, cursorY, textW, stampRect.height()),
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

    const int margin = 8;
    const int padding = 10;
    const int maxWidth = static_cast<int>(option.rect.width() * 0.70);

    QFont bodyFont = option.font;
    bodyFont.setPointSize(std::max(9, bodyFont.pointSize()));

    QFont metaFont = option.font;
    metaFont.setPointSize(std::max(8, metaFont.pointSize() - 1));

    QFontMetrics bodyFm(bodyFont);
    QFontMetrics metaFm(metaFont);

    QString header;
    if (m_showAuthors && !outgoing && !author.isEmpty()) {
        header = author;
    }

    const QRect textRect0(0, 0, maxWidth, 10'000);
    const QRect headerRect = header.isEmpty() ? QRect() : metaFm.boundingRect(textRect0, Qt::TextWordWrap, header);
    const QRect bodyRect = bodyFm.boundingRect(textRect0, Qt::TextWordWrap, body);
    const QRect stampRect = metaFm.boundingRect(textRect0, Qt::TextSingleLine, stamp);

    int contentW = std::max({bodyRect.width(), headerRect.width(), stampRect.width()});
    int contentH = bodyRect.height() + stampRect.height();
    if (!header.isEmpty()) {
        contentH += headerRect.height();
    }

    const int bubbleW = contentW + padding * 2;
    const int bubbleH = contentH + padding * 2;

    return {bubbleW + margin * 2, bubbleH + margin * 2};
}

} // namespace vox::ui::conversations

