#include <QApplication>

#include "qt/QtUiTests.hpp"

int main(int argc, char **argv) {
  QApplication app(argc, argv);

  int status = 0;
  {
    ConversationListModelQtTests test;
    status |= QTest::qExec(&test, argc, argv);
  }
  {
    MainWindowQtTests test;
    status |= QTest::qExec(&test, argc, argv);
  }

  return status;
}
