#include "ui/auth/RegisterPage.hpp"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace vox::ui::auth {

RegisterPage::RegisterPage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);

    auto *formLayout = new QFormLayout;
    m_username = new QLineEdit(this);
    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);

    formLayout->addRow("Username", m_username);
    formLayout->addRow("Password", m_password);

    auto *buttonsLayout = new QHBoxLayout;
    auto *back = new QPushButton("Back", this);
    auto *submit = new QPushButton("Register", this);

    buttonsLayout->addWidget(back);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(submit);

    layout->addLayout(formLayout);
    layout->addLayout(buttonsLayout);

    connect(back, &QPushButton::clicked, this, &RegisterPage::backRequested);
    connect(submit, &QPushButton::clicked, this, [this]() {
        emit submitRegister(m_username->text().trimmed(), m_password->text());
    });
}

} // namespace vox::ui::auth
