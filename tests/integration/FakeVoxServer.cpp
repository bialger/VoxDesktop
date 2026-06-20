#include "integration/FakeVoxServer.hpp"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrlQuery>

namespace network = vox::network;

namespace {

constexpr int kHttpOk = 200;
constexpr int kHttpBadRequest = 400;
constexpr int kHttpUnauthorized = 401;
constexpr int kHttpNotFound = 404;
constexpr int kHttpConflict = 409;
constexpr int kHttpServiceUnavailable = 503;

constexpr int kErrorCodeQueueFull = 9;

constexpr qint64 kFakeConversationCreatedAt = 1710000000;

QString TrimQuery(const QString &path) {
  const qsizetype q = path.indexOf('?');
  return q >= 0 ? path.left(q) : path;
}

QUrlQuery QueryFromPath(const QString &path) {
  const qsizetype q = path.indexOf('?');
  if (q < 0) {
    return {};
  }
  return QUrlQuery(path.mid(q + 1));
}

} // namespace

network::NetworkResponse FakeVoxServer::Handle(const QString &method,
                                               const QString &path,
                                               const QByteArray &body,
                                               const network::NetworkAccess::HeaderMap &headers) {
  const QString route = TrimQuery(path);

  if (route == "/v1/health" && method == "GET") {
    return Ok(QJsonObject{{"status", "ok"}});
  }

  if (route == "/v1/register" && method == "POST") {
    const auto req = ParseJson(body);
    if (!req.has_value()) {
      return Error(kHttpBadRequest, 1, "Invalid JSON");
    }

    const QString username = req->value("username").toString();
    const QString password = req->value("password_derived_value").toString();
    const QString device_id = req->value("device_id").toString();
    if (username.isEmpty() || password.isEmpty() || device_id.isEmpty()) {
      return Error(kHttpBadRequest, 1, "Missing fields");
    }

    if (m_usersByUsername_.contains(username)) {
      return Error(kHttpConflict, 4, "Username exists");
    }

    AccountState account;
    account.userId = QString("usr_%1").arg(m_userSeq_++);
    account.username = username;
    account.passwordDerived = password;
    account.deviceId = device_id;
    account.accessToken = QString("acc_%1").arg(account.userId);
    account.refreshToken = QString("ref_%1").arg(account.userId);

    m_usersByUsername_.insert(username, account);
    m_userByAccessToken_.insert(account.accessToken, account.userId);
    m_userByRefreshToken_.insert(account.refreshToken, account.userId);

    return Ok(QJsonObject{{"user_id", account.userId},
                          {"access_token", account.accessToken},
                          {"refresh_token", account.refreshToken},
                          {"device_status", "created"},
                          {"sync_key_version", 1}});
  }

  if (route == "/v1/login" && method == "POST") {
    const auto req = ParseJson(body);
    if (!req.has_value()) {
      return Error(kHttpBadRequest, 1, "Invalid JSON");
    }

    const QString username = req->value("username").toString();
    const QString password = req->value("password_derived_value").toString();
    const QString device_id = req->value("device_id").toString();

    if (!m_usersByUsername_.contains(username)) {
      return Error(kHttpUnauthorized, 2, "Unauthorized");
    }

    auto account = m_usersByUsername_.value(username);
    if (account.passwordDerived != password) {
      return Error(kHttpUnauthorized, 2, "Unauthorized");
    }

    account.deviceId = device_id;
    account.accessToken =
        QString("acc_%1_%2").arg(account.userId, QString::number(QDateTime::currentMSecsSinceEpoch()));
    account.refreshToken =
        QString("ref_%1_%2").arg(account.userId, QString::number(QDateTime::currentMSecsSinceEpoch()));
    m_usersByUsername_.insert(username, account);
    m_userByAccessToken_.insert(account.accessToken, account.userId);
    m_userByRefreshToken_.insert(account.refreshToken, account.userId);

    return Ok(QJsonObject{{"user_id", account.userId},
                          {"access_token", account.accessToken},
                          {"refresh_token", account.refreshToken},
                          {"device_status", "existing"},
                          {"sync_key_version", 1}});
  }

  if (route == "/v1/refresh" && method == "POST") {
    const auto req = ParseJson(body);
    if (!req.has_value()) {
      return Error(kHttpBadRequest, 1, "Invalid JSON");
    }

    const QString refresh_token = req->value("refresh_token").toString();
    if (!m_userByRefreshToken_.contains(refresh_token)) {
      return Error(kHttpUnauthorized, 2, "Unauthorized");
    }

    const QString user_id = m_userByRefreshToken_.value(refresh_token);
    const QString access = QString("acc_%1_ref").arg(user_id);
    const QString refresh = QString("ref_%1_ref").arg(user_id);
    m_userByAccessToken_.insert(access, user_id);
    m_userByRefreshToken_.insert(refresh, user_id);

    return Ok(QJsonObject{{"access_token", access}, {"refresh_token", refresh}});
  }

  if (route == "/v1/logout" && method == "POST") {
    return Ok(QJsonObject{});
  }

  if (route == "/v1/me" && method == "GET") {
    const QString user_id = AuthUserFromHeaders(headers);
    if (user_id.isEmpty()) {
      return Error(kHttpUnauthorized, 2, "Unauthorized");
    }

    for (auto &it : m_usersByUsername_) {
      if (it.userId == user_id) {
        return Ok(QJsonObject{{"user_id", it.userId},
                              {"username", it.username},
                              {"current_device_id", it.deviceId},
                              {"sync_key_version", 1}});
      }
    }

    return Error(kHttpNotFound, 3, "Not found");
  }

  if (m_forceUnauthorized_) {
    return Error(kHttpUnauthorized, 2, "Unauthorized");
  }

  const QString user_id = AuthUserFromHeaders(headers);
  if (user_id.isEmpty()) {
    return Error(kHttpUnauthorized, 2, "Unauthorized");
  }

  if (route == "/v1/conversations" && method == "GET") {
    QJsonArray conversations;
    conversations.push_back(QJsonObject{{"conversation_id", "conv_1"},
                                        {"type", 0},
                                        {"created_by", user_id},
                                        {"created_by_username", "alice"},
                                        {"peer_user_id", "usr_peer"},
                                        {"created_at", kFakeConversationCreatedAt},
                                        {"membership_version", 1},
                                        {"my_role", "member"}});
    return Ok(QJsonObject{{"conversations", conversations}});
  }

  if (route == "/v1/messages/send" && method == "POST") {
    if (m_failNextSend_) {
      m_failNextSend_ = false;
      return Error(kHttpServiceUnavailable, kErrorCodeQueueFull, "Queue full");
    }

    const auto req = ParseJson(body);
    if (!req.has_value()) {
      return Error(kHttpBadRequest, 1, "Invalid JSON");
    }

    Envelope env;
    env.envelopeId = req->value("envelope_id").toString();
    env.conversationId = req->value("conversation_id").toString();
    env.senderUserId = user_id;
    env.senderDeviceId = req->value("device_id").toString();
    env.ciphertext = req->value("ciphertext").toString();
    env.serverTimestamp = QDateTime::currentSecsSinceEpoch();
    env.envelopeType = req->value("envelope_type").toInt();

    if (env.envelopeId.isEmpty()) {
      env.envelopeId = QString("env_%1").arg(m_envelopeSeq_++);
    }
    m_envelopes_.push_back(env);

    return Ok(QJsonObject{{"envelope_id", env.envelopeId},
                          {"server_timestamp", static_cast<double>(env.serverTimestamp)},
                          {"delivered_to_count", 1}});
  }

  if (route == "/v1/messages/ack" && method == "POST") {
    return Ok(QJsonObject{});
  }

  if (route == "/v1/sync/pending" && method == "GET") {
    QJsonArray envelopes;
    for (const auto &env : m_envelopes_) {
      envelopes.push_back(QJsonObject{{"envelope_id", env.envelopeId},
                                      {"conversation_id", env.conversationId},
                                      {"sender_user_id", env.senderUserId},
                                      {"sender_device_id", env.senderDeviceId},
                                      {"ciphertext", env.ciphertext},
                                      {"server_timestamp", static_cast<double>(env.serverTimestamp)},
                                      {"envelope_type", env.envelopeType}});
    }

    return Ok(QJsonObject{{"envelopes", envelopes}, {"next_cursor", ""}, {"has_more", false}});
  }

  if (route == "/v1/attachments/upload-init" && method == "POST") {
    const QString id = QString("att_%1").arg(m_attachmentSeq_++);
    m_attachments_.insert(id, {});
    return Ok(QJsonObject{{"attachment_id", id}, {"blob_path", "/blob/" + id}});
  }

  if (route.startsWith("/v1/attachments/") && route.endsWith("/chunk") && method == "PUT") {
    const QString attachment_id = route.mid(QString("/v1/attachments/").size());
    const QString id = attachment_id.left(attachment_id.indexOf("/chunk"));
    auto stored = m_attachments_.value(id);

    const auto query = QueryFromPath(path);
    const qint64 offset = query.queryItemValue("offset").toLongLong();
    if (offset < 0) {
      return Error(kHttpBadRequest, 1, "Bad offset");
    }

    if (stored.size() < offset) {
      stored.append(QByteArray(static_cast<int>(offset - stored.size()), '\0'));
    }
    if (stored.size() == offset) {
      stored.append(body);
    } else {
      stored.replace(static_cast<int>(offset), body.size(), body);
    }

    m_attachments_[id] = stored;
    return Ok(QJsonObject{});
  }

  if (route.startsWith("/v1/attachments/") && route.endsWith("/finalize") && method == "POST") {
    return Ok(QJsonObject{});
  }

  if (route.startsWith("/v1/attachments/") && method == "GET") {
    const QString id = route.mid(QString("/v1/attachments/").size());
    return {.statusCode = kHttpOk, .body = m_attachments_.value(id), .errorMessage = {}};
  }

  if (route.startsWith("/v1/users/by-username/") && method == "GET") {
    const QString username = route.mid(QString("/v1/users/by-username/").size());
    if (!m_usersByUsername_.contains(username)) {
      return Error(kHttpNotFound, 3, "Not found");
    }
    const auto state = m_usersByUsername_.value(username);
    return Ok(QJsonObject{{"user_id", state.userId}, {"username", state.username}});
  }

  if (route == "/v1/users/search" && method == "GET") {
    QJsonArray users;
    for (auto &it : m_usersByUsername_) {
      users.push_back(QJsonObject{{"user_id", it.userId}, {"username", it.username}});
    }
    return Ok(QJsonObject{{"users", users}});
  }

  if (route.startsWith("/v1/users/") && route.endsWith("/devices") && method == "GET") {
    const QString user_id_in_path = route.mid(QString("/v1/users/").size());
    const QString user_id = user_id_in_path.left(user_id_in_path.indexOf("/devices"));

    QJsonArray devices;
    for (auto &it : m_usersByUsername_) {
      if (it.userId == user_id) {
        devices.push_back(QJsonObject{{"device_id", it.deviceId},
                                      {"device_label", "Desktop"},
                                      {"identity_key_public", "idpk"},
                                      {"signed_prekey_public", "spk"},
                                      {"signed_prekey_signature", "sig"}});
      }
    }
    return Ok(QJsonObject{{"devices", devices}});
  }

  if (route == "/v1/sync/key-bundle" && method == "GET") {
    return Ok(QJsonObject{{"version", 1},
                          {"wrapped_sync_key", "wrapped"},
                          {"sync_wrap_salt", "salt"},
                          {"sync_wrap_params", QJsonObject{{"algorithm", "argon2id"}}}});
  }

  if (route == "/v1/sync/key-bundle" && method == "PUT") {
    return Ok(QJsonObject{});
  }

  if (route == "/v1/sync/changes" && method == "GET") {
    return Ok(QJsonObject{{"records", QJsonArray{}}, {"next_cursor", ""}, {"has_more", false}});
  }

  if (route.startsWith("/v1/sync/records/") && (method == "PUT" || method == "DELETE")) {
    return Ok(QJsonObject{});
  }

  return Error(kHttpNotFound, 3, "Not found");
}

void FakeVoxServer::SetForceUnauthorized(bool value) {
  m_forceUnauthorized_ = value;
}

void FakeVoxServer::SetFailNextSend(bool value) {
  m_failNextSend_ = value;
}

network::NetworkResponse FakeVoxServer::Ok(const QJsonObject &obj, int status) const {
  return {.statusCode = status, .body = QJsonDocument(obj).toJson(QJsonDocument::Compact), .errorMessage = {}};
}

network::NetworkResponse FakeVoxServer::Error(int status, int code, const QString &message) const {
  const QJsonObject body{{"error", QJsonObject{{"code", code}, {"message", message}}}};
  return {.statusCode = status, .body = QJsonDocument(body).toJson(QJsonDocument::Compact), .errorMessage = message};
}

std::optional<QJsonObject> FakeVoxServer::ParseJson(QByteArrayView bytes) const {
  const auto doc = QJsonDocument::fromJson(bytes.toByteArray());
  if (!doc.isObject()) {
    return std::nullopt;
  }
  return doc.object();
}

QString FakeVoxServer::AuthUserFromHeaders(const network::NetworkAccess::HeaderMap &headers) const {
  if (m_forceUnauthorized_) {
    return {};
  }

  const QString authorization = headers.value("Authorization");
  if (!authorization.startsWith("Bearer ")) {
    return {};
  }

  const QString token = authorization.mid(QString("Bearer ").size());
  return m_userByAccessToken_.value(token);
}
