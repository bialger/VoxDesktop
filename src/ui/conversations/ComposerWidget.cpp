#include "ui/conversations/ComposerWidget.hpp"

#include <QHBoxLayout>

#include <memory>

namespace vox::ui::conversations {
namespace {

constexpr int kComposerEditHeightPx = 80;

} // namespace

ComposerWidget::ComposerWidget(QWidget *parent) : QWidget(parent), m_edit(new QPlainTextEdit(this)) {
  auto layout = std::make_unique<QHBoxLayout>();
  layout->setContentsMargins(0, 0, 0, 0);

  m_edit->setPlaceholderText("Write a message");
  m_edit->setFixedHeight(kComposerEditHeightPx);

  auto send_button = std::make_unique<QPushButton>("Send", this);
  m_sendButton = send_button.release();

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
  setLayout(layout.release());
}

} // namespace vox::ui::conversations
