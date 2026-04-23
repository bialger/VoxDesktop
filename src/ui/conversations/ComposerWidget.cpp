#include "ui/conversations/ComposerWidget.hpp"

#include <QHBoxLayout>

namespace vox::ui::conversations {

ComposerWidget::ComposerWidget(QWidget *parent) : QWidget(parent) {
  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  m_edit = new QPlainTextEdit(this);
  m_edit->setPlaceholderText("Write a message");
  m_edit->setFixedHeight(80);

  m_sendButton = new QPushButton("Send", this);

  layout->addWidget(m_edit, 1);
  layout->addWidget(m_sendButton);

  connect(m_sendButton, &QPushButton::clicked, this, [this]() {
    const QString text = m_edit->toPlainText().trimmed();
    if (text.isEmpty()) {
      return;
    }
    emit sendRequested(text);
    m_edit->clear();
  });
}

} // namespace vox::ui::conversations
