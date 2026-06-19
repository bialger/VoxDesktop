#include "network/NetworkAccess.hpp"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

namespace vox::network {
namespace {

constexpr int kHttpStatusOkMin = 200;
constexpr int kHttpStatusOkMaxExclusive = 300;
constexpr int kNetworkTransferTimeoutMs = 15000;

} // namespace

bool NetworkResponse::ok() const {
  return statusCode >= kHttpStatusOkMin && statusCode < kHttpStatusOkMaxExclusive && errorMessage.isEmpty();
}

NetworkAccess::NetworkAccess(QString baseUrl) : m_baseUrl(std::move(baseUrl)) {
}

void NetworkAccess::setBearerToken(QString token) {
  m_bearerToken = std::move(token);
}

void NetworkAccess::setCustomHandler(Handler handler) {
  m_customHandler = std::move(handler);
}

NetworkResponse NetworkAccess::get(const QString &path, const QUrlQuery &query, const HeaderMap &headers) const {
  return execute("GET", path, {}, query, headers, {});
}

NetworkResponse NetworkAccess::postJson(const QString &path,
                                        const QByteArray &jsonBody,
                                        const QUrlQuery &query,
                                        const HeaderMap &headers) const {
  return execute("POST", path, jsonBody, query, headers, "application/json");
}

NetworkResponse NetworkAccess::putJson(const QString &path,
                                       const QByteArray &jsonBody,
                                       const QUrlQuery &query,
                                       const HeaderMap &headers) const {
  return execute("PUT", path, jsonBody, query, headers, "application/json");
}

NetworkResponse NetworkAccess::putBytes(const QString &path,
                                        const QByteArray &payload,
                                        const QUrlQuery &query,
                                        const HeaderMap &headers) const {
  return execute("PUT", path, payload, query, headers, "application/octet-stream");
}

NetworkResponse NetworkAccess::postBytes(const QString &path,
                                         const QByteArray &payload,
                                         const QUrlQuery &query,
                                         const HeaderMap &headers) const {
  return execute("POST", path, payload, query, headers, "application/octet-stream");
}

NetworkResponse NetworkAccess::deleteRequest(const QString &path,
                                             const QUrlQuery &query,
                                             const HeaderMap &headers) const {
  return execute("DELETE", path, {}, query, headers, {});
}

NetworkResponse NetworkAccess::execute(const QString &method,
                                       const QString &path,
                                       const QByteArray &body,
                                       const QUrlQuery &query,
                                       const HeaderMap &headers,
                                       const QByteArray &contentType) const {
  QString normalized_path = path;
  if (!normalized_path.startsWith('/')) {
    normalized_path.prepend('/');
  }

  if (m_customHandler) {
    QString full_path = normalized_path;
    if (!query.isEmpty()) {
      full_path += "?" + query.toString(QUrl::FullyEncoded);
    }

    HeaderMap effective_headers = headers;
    if (!m_bearerToken.isEmpty()) {
      effective_headers.insert("Authorization", "Bearer " + m_bearerToken);
    }
    if (!contentType.isEmpty()) {
      effective_headers.insert("Content-Type", QString::fromUtf8(contentType));
    }
    return m_customHandler(method, full_path, body, effective_headers);
  }

  QUrl url(m_baseUrl + normalized_path);
  url.setQuery(query);

  QNetworkRequest request(url);
  request.setTransferTimeout(kNetworkTransferTimeoutMs);

  if (!m_bearerToken.isEmpty()) {
    request.setRawHeader("Authorization", ("Bearer " + m_bearerToken).toUtf8());
  }
  for (auto it = headers.begin(); it != headers.end(); ++it) {
    request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
  }
  if (!contentType.isEmpty()) {
    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
  }

  QNetworkAccessManager manager;
  QNetworkReply *reply = nullptr;
  if (method == "GET") {
    reply = manager.get(request);
  } else if (method == "POST") {
    reply = manager.post(request, body);
  } else if (method == "PUT") {
    reply = manager.put(request, body);
  } else if (method == "DELETE") {
    reply = manager.deleteResource(request);
  } else {
    return {0, {}, "Unsupported HTTP method"};
  }

  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  QTimer timeout;
  timeout.setSingleShot(true);
  QObject::connect(&timeout, &QTimer::timeout, [&]() {
    if (reply->isRunning()) {
      reply->abort();
    }
    loop.quit();
  });
  timeout.start(kNetworkTransferTimeoutMs);
  loop.exec();

  NetworkResponse result;
  result.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  result.body = reply->readAll();

  if (reply->error() != QNetworkReply::NoError && result.statusCode == 0) {
    result.errorMessage = reply->errorString();
  }

  reply->deleteLater();
  return result;
}

} // namespace vox::network
