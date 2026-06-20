#include "ui/auth/RegisterPage.hpp"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <memory>

namespace vox::ui::auth {

RegisterPage::RegisterPage(QWidget *parent) :
    QWidget(parent), m_username(new QLineEdit(this)), m_password(new QLineEdit(this)) {
  auto layout = std::make_unique<QVBoxLayout>();

  auto form_layout = std::make_unique<QFormLayout>();

  m_password->setEchoMode(QLineEdit::Password);

  form_layout->addRow("Username", m_username);
  form_layout->addRow("Password", m_password);

  auto buttons_layout = std::make_unique<QHBoxLayout>();
  auto *back = new QPushButton("Back", this);
  auto *submit = new QPushButton("Register", this);

  buttons_layout->addWidget(back);
  buttons_layout->addStretch(1);
  buttons_layout->addWidget(submit);

  layout->addLayout(form_layout.release());
  layout->addLayout(buttons_layout.release());
  setLayout(layout.release());

  connect(back, &QPushButton::clicked, this, &RegisterPage::backRequested);
  connect(submit, &QPushButton::clicked, this, [this]() {
    emit submitRegister(m_username->text().trimmed(), m_password->text());
  });
}

} // namespace vox::ui::auth
