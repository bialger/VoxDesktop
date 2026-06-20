#ifndef VOX_TESTS_QT_QTUITESTS_HPP
#define VOX_TESTS_QT_QTUITESTS_HPP

#include <QObject>
#include <QSignalSpy>
#include <QTest>

#include "domain/Models.hpp"
#include "ui/conversations/ConversationListModel.hpp"
#include "ui/shell/MainWindow.hpp"

class ConversationListModelQtTests : public QObject {
  Q_OBJECT

private slots:
  void SetConversationsEmitsModelResetAndExposesRoles() {
    constexpr int kExpectedUnreadCount = 5;

    vox::ui::conversations::ConversationListModel model;
    QSignalSpy spy(&model, &QAbstractItemModel::modelReset);

    vox::domain::Conversation conversation;
    conversation.conversationId = "conv_1";
    conversation.title = "General";
    conversation.unreadCount = kExpectedUnreadCount;

    model.setConversations({conversation});

    QCOMPARE(spy.count(), 1);
    QCOMPARE(model.rowCount(), 1);

    const QModelIndex index = model.index(0, 0);
    QCOMPARE(index.data(vox::ui::conversations::ConversationListModel::ConversationIdRole).toString(),
             QString("conv_1"));
    QCOMPARE(index.data(vox::ui::conversations::ConversationListModel::TitleRole).toString(), QString("General"));
    QCOMPARE(index.data(vox::ui::conversations::ConversationListModel::UnreadCountRole).toInt(), kExpectedUnreadCount);
  }
};

class MainWindowQtTests : public QObject {
  Q_OBJECT

private slots:
  void MainWindowConstructsCoreWidgets() {
    vox::ui::shell::MainWindow window;
    QVERIFY(window.centralWidget() != nullptr);

    QVERIFY(window.isVisible() == false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.isVisible());
  }
};

#endif // VOX_TESTS_QT_QTUITESTS_HPP
