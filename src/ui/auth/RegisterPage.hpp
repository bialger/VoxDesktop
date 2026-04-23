#ifndef VOX_UI_AUTH_REGISTERPAGE_HPP
#define VOX_UI_AUTH_REGISTERPAGE_HPP

#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

namespace vox::ui::auth {

class RegisterPage final : public QWidget {
    Q_OBJECT

public:
    explicit RegisterPage(QWidget *parent = nullptr);

signals:
    void submitRegister(const QString &username, const QString &passwordDerived);
    void backRequested();

private:
    QLineEdit *m_username{nullptr};
    QLineEdit *m_password{nullptr};
};

} // namespace vox::ui::auth

#endif // VOX_UI_AUTH_REGISTERPAGE_HPP
