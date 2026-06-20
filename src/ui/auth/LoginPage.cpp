#include "ui/auth/LoginPage.hpp"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <memory>

namespace vox::ui::auth {

LoginPage::LoginPage(QWidget *parent) :
    QWidget(parent), m_username(new QLineEdit(this)), m_password(new QLineEdit(this)) {
  auto layout = std::make_unique<QVBoxLayout>();

  auto form_layout = std::make_unique<QFormLayout>();

  m_password->setEchoMode(QLineEdit::Password);

  form_layout->addRow("Username", m_username);
  form_layout->addRow("Password", m_password);

  auto buttons_layout = std::make_unique<QHBoxLayout>();
  auto *back = new QPushButton("Back", this);
  auto *login = new QPushButton("Login", this);

  buttons_layout->addWidget(back);
  buttons_layout->addStretch(1);
  buttons_layout->addWidget(login);

  layout->addLayout(form_layout.release());
  layout->addLayout(buttons_layout.release());
  setLayout(layout.release());

  connect(back, &QPushButton::clicked, this, &LoginPage::backRequested);
  connect(login, &QPushButton::clicked, this, [this]() {
    emit submitLogin(m_username->text().trimmed(), m_password->text());
  });
}

} // namespace vox::ui::auth
