#ifndef VOX_UI_AUTH_LOGINPAGE_HPP
#define VOX_UI_AUTH_LOGINPAGE_HPP

#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

namespace vox::ui::auth {

class LoginPage final : public QWidget {
    Q_OBJECT

public:
    explicit LoginPage(QWidget *parent = nullptr);

signals:
    void submitLogin(const QString &username, const QString &passwordDerived);
    void backRequested();

private:
    QLineEdit *m_username{nullptr};
    QLineEdit *m_password{nullptr};
};

} // namespace vox::ui::auth

#endif // VOX_UI_AUTH_LOGINPAGE_HPP
