#include "ui/auth/LoginPage.hpp"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace vox::ui::auth {

LoginPage::LoginPage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);

    auto *formLayout = new QFormLayout;
    m_username = new QLineEdit(this);
    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);

    formLayout->addRow("Username", m_username);
    formLayout->addRow("Password", m_password);

    auto *buttonsLayout = new QHBoxLayout;
    auto *back = new QPushButton("Back", this);
    auto *login = new QPushButton("Login", this);

    buttonsLayout->addWidget(back);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(login);

    layout->addLayout(formLayout);
    layout->addLayout(buttonsLayout);

    connect(back, &QPushButton::clicked, this, &LoginPage::backRequested);
    connect(login, &QPushButton::clicked, this, [this]() {
        emit submitLogin(m_username->text().trimmed(), m_password->text());
    });
}

} // namespace vox::ui::auth
