#ifndef VOX_UI_AUTH_WELCOMEPAGE_HPP
#define VOX_UI_AUTH_WELCOMEPAGE_HPP

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

#include <QFutureWatcher>

namespace vox::ui::auth {

class WelcomePage final : public QWidget {
  Q_OBJECT

public:
  explicit WelcomePage(QWidget *parent = nullptr);

  [[nodiscard]] QString selectedBaseUrl() const;

signals:
  void loginRequested();
  void registerRequested();
  void serverBaseUrlReady(const QString &baseUrl);

private slots:
  void onConnectClicked();
  void onHealthFinished();

private:
  void setUiBusy(bool busy);
  void setStatusOk(const QString &message);
  void setStatusError(const QString &message);

  QLineEdit *m_baseUrlEdit{nullptr};
  QPushButton *m_connectButton{nullptr};
  QPushButton *m_loginButton{nullptr};
  QPushButton *m_registerButton{nullptr};
  QLabel *m_statusLabel{nullptr};
  QFutureWatcher<bool> m_healthWatcher;
  QString m_selectedBaseUrl;
};

} // namespace vox::ui::auth

#endif // VOX_UI_AUTH_WELCOMEPAGE_HPP
