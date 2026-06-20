#include "ui/auth/WelcomePage.hpp"

#include "network/ApiPaths.hpp"
#include "network/JsonCodec.hpp"
#include "network/NetworkAccess.hpp"

#include <QtConcurrent>

#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>

#include <memory>

namespace vox::ui::auth {
namespace {

constexpr int kTitleFontPointSize = 20;
constexpr int kSectionSpacingPx = 16;
constexpr int kButtonSectionSpacingPx = 12;

QString NormalizeBaseUrl(QString url) {
  url = url.trimmed();
  while (url.endsWith('/')) {
    url.chop(1);
  }
  return url;
}

bool CheckHealthSync(const QString &baseUrl) {
  network::NetworkAccess net(baseUrl);
  const auto response = net.get(network::ApiPaths::kHealth);
  if (!response.ok()) {
    return false;
  }
  const auto object = network::JsonCodec::parseObject(response.body);
  return object.has_value() && object->value("status").toString() == "ok";
}

} // namespace

WelcomePage::WelcomePage(QWidget *parent) :
    QWidget(parent), m_baseUrlEdit(new QLineEdit(this)), m_statusLabel(new QLabel(this)) {
  auto layout = std::make_unique<QVBoxLayout>();
  layout->setAlignment(Qt::AlignCenter);

  auto *title = new QLabel("Vox Messenger", this);
  QFont font = title->font();
  font.setPointSize(kTitleFontPointSize);
  font.setBold(true);
  title->setFont(font);

  m_baseUrlEdit->setPlaceholderText("Server URL, e.g. http://127.0.0.1:8080");

  const QString default_url = qEnvironmentVariableIsSet("VOX_BASE_URL") ? QString::fromUtf8(qgetenv("VOX_BASE_URL"))
                                                                        : QStringLiteral("http://127.0.0.1:8080");
  QSettings settings;
  const QString remembered = settings.value("vox/server_base_url").toString();
  m_baseUrlEdit->setText(remembered.isEmpty() ? default_url : remembered);

  auto connect_button = std::make_unique<QPushButton>("Connect", this);
  m_connectButton = connect_button.release();

  m_statusLabel->setText("Not connected");

  auto login_button = std::make_unique<QPushButton>("Login", this);
  m_loginButton = login_button.release();
  auto register_button = std::make_unique<QPushButton>("Register", this);
  m_registerButton = register_button.release();
  m_loginButton->setEnabled(false);
  m_registerButton->setEnabled(false);

  layout->addWidget(title);
  layout->addSpacing(kSectionSpacingPx);
  layout->addWidget(m_baseUrlEdit);
  layout->addWidget(m_connectButton);
  layout->addWidget(m_statusLabel);
  layout->addSpacing(kButtonSectionSpacingPx);
  layout->addWidget(m_loginButton);
  layout->addWidget(m_registerButton);

  connect(m_connectButton, &QPushButton::clicked, this, &WelcomePage::onConnectClicked);
  connect(m_baseUrlEdit, &QLineEdit::returnPressed, this, &WelcomePage::onConnectClicked);
  connect(&m_healthWatcher, &QFutureWatcher<bool>::finished, this, &WelcomePage::onHealthFinished);

  connect(m_loginButton, &QPushButton::clicked, this, &WelcomePage::loginRequested);
  connect(m_registerButton, &QPushButton::clicked, this, &WelcomePage::registerRequested);
  setLayout(layout.release());
}

QString WelcomePage::selectedBaseUrl() const {
  return m_selectedBaseUrl;
}

void WelcomePage::onConnectClicked() {
  if (m_healthWatcher.isRunning()) {
    return;
  }

  const QString base_url = NormalizeBaseUrl(m_baseUrlEdit->text());
  m_baseUrlEdit->setText(base_url);

  if (!(base_url.startsWith("http://") || base_url.startsWith("https://"))) {
    setStatusError("URL must start with http:// or https://");
    return;
  }

  setUiBusy(true);
  m_statusLabel->setText("Checking server health…");

  m_healthWatcher.setFuture(QtConcurrent::run([base_url]() { return CheckHealthSync(base_url); }));
}

void WelcomePage::onHealthFinished() {
  setUiBusy(false);

  const bool ok = m_healthWatcher.result();
  const QString base_url = NormalizeBaseUrl(m_baseUrlEdit->text());
  if (!ok) {
    setStatusError("Health check failed. Verify URL and server status.");
    return;
  }

  m_selectedBaseUrl = base_url;
  QSettings settings;
  settings.setValue("vox/server_base_url", base_url);

  setStatusOk("Connected");
  m_loginButton->setEnabled(true);
  m_registerButton->setEnabled(true);
  emit serverBaseUrlReady(base_url);
}

void WelcomePage::setUiBusy(bool busy) {
  m_baseUrlEdit->setEnabled(!busy);
  m_connectButton->setEnabled(!busy);
}

void WelcomePage::setStatusOk(const QString &message) {
  m_statusLabel->setStyleSheet("color: #2e7d32;"); // green-ish
  m_statusLabel->setText(message);
}

void WelcomePage::setStatusError(const QString &message) {
  m_statusLabel->setStyleSheet("color: #c62828;"); // red-ish
  m_statusLabel->setText(message);
  m_loginButton->setEnabled(false);
  m_registerButton->setEnabled(false);
  m_selectedBaseUrl.clear();
}

} // namespace vox::ui::auth
