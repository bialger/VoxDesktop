#include "ui/auth/WelcomePage.hpp"

#include "network/ApiPaths.hpp"
#include "network/JsonCodec.hpp"
#include "network/NetworkAccess.hpp"

#include <QtConcurrent>

#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>

namespace vox::ui::auth {
namespace {

QString normalizeBaseUrl(QString url) {
    url = url.trimmed();
    while (url.endsWith('/')) {
        url.chop(1);
    }
    return url;
}

bool checkHealthSync(const QString &baseUrl) {
    network::NetworkAccess net(baseUrl);
    const auto response = net.get(network::ApiPaths::kHealth);
    if (!response.ok()) {
        return false;
    }
    const auto object = network::JsonCodec::parseObject(response.body);
    return object.has_value() && object->value("status").toString() == "ok";
}

} // namespace

WelcomePage::WelcomePage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel("Vox Messenger", this);
    QFont font = title->font();
    font.setPointSize(20);
    font.setBold(true);
    title->setFont(font);

    m_baseUrlEdit = new QLineEdit(this);
    m_baseUrlEdit->setPlaceholderText("Server URL, e.g. http://127.0.0.1:8080");

    const QString defaultUrl = qEnvironmentVariableIsSet("VOX_BASE_URL")
                                   ? QString::fromUtf8(qgetenv("VOX_BASE_URL"))
                                   : QStringLiteral("http://127.0.0.1:8080");
    QSettings settings;
    const QString remembered = settings.value("vox/server_base_url").toString();
    m_baseUrlEdit->setText(remembered.isEmpty() ? defaultUrl : remembered);

    m_connectButton = new QPushButton("Connect", this);
    m_statusLabel = new QLabel(this);
    m_statusLabel->setText("Not connected");

    m_loginButton = new QPushButton("Login", this);
    m_registerButton = new QPushButton("Register", this);
    m_loginButton->setEnabled(false);
    m_registerButton->setEnabled(false);

    layout->addWidget(title);
    layout->addSpacing(16);
    layout->addWidget(m_baseUrlEdit);
    layout->addWidget(m_connectButton);
    layout->addWidget(m_statusLabel);
    layout->addSpacing(12);
    layout->addWidget(m_loginButton);
    layout->addWidget(m_registerButton);

    connect(m_connectButton, &QPushButton::clicked, this, &WelcomePage::onConnectClicked);
    connect(m_baseUrlEdit, &QLineEdit::returnPressed, this, &WelcomePage::onConnectClicked);
    connect(&m_healthWatcher, &QFutureWatcher<bool>::finished, this, &WelcomePage::onHealthFinished);

    connect(m_loginButton, &QPushButton::clicked, this, &WelcomePage::loginRequested);
    connect(m_registerButton, &QPushButton::clicked, this, &WelcomePage::registerRequested);
}

QString WelcomePage::selectedBaseUrl() const {
    return m_selectedBaseUrl;
}

void WelcomePage::onConnectClicked() {
    if (m_healthWatcher.isRunning()) {
        return;
    }

    const QString baseUrl = normalizeBaseUrl(m_baseUrlEdit->text());
    m_baseUrlEdit->setText(baseUrl);

    if (!(baseUrl.startsWith("http://") || baseUrl.startsWith("https://"))) {
        setStatusError("URL must start with http:// or https://");
        return;
    }

    setUiBusy(true);
    m_statusLabel->setText("Checking server health…");

    m_healthWatcher.setFuture(QtConcurrent::run([baseUrl]() { return checkHealthSync(baseUrl); }));
}

void WelcomePage::onHealthFinished() {
    setUiBusy(false);

    const bool ok = m_healthWatcher.result();
    const QString baseUrl = normalizeBaseUrl(m_baseUrlEdit->text());
    if (!ok) {
        setStatusError("Health check failed. Verify URL and server status.");
        return;
    }

    m_selectedBaseUrl = baseUrl;
    QSettings settings;
    settings.setValue("vox/server_base_url", baseUrl);

    setStatusOk("Connected");
    m_loginButton->setEnabled(true);
    m_registerButton->setEnabled(true);
    emit serverBaseUrlReady(baseUrl);
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
