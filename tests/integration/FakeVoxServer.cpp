#include "integration/FakeVoxServer.hpp"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrlQuery>

namespace network = vox::network;

namespace {

QString trimQuery(const QString &path) {
  const int q = path.indexOf('?');
  return q >= 0 ? path.left(q) : path;
}

QUrlQuery queryFromPath(const QString &path) {
  const int q = path.indexOf('?');
  if (q < 0) {
    return {};
  }
  return QUrlQuery(path.mid(q + 1));
}

} // namespace

network::NetworkResponse FakeVoxServer::handle(const QString &method,
                                               const QString &path,
                                               const QByteArray &body,
                                               const network::NetworkAccess::HeaderMap &headers) {
  const QString route = trimQuery(path);

  if (route == "/v1/health" && method == "GET") {
    return ok(QJsonObject{{"status", "ok"}});
  }

  if (route == "/v1/register" && method == "POST") {
    const auto req = parseJson(body);
    if (!req.has_value()) {
      return error(400, 1, "Invalid JSON");
    }

    const QString username = req->value("username").toString();
    const QString password = req->value("password_derived_value").toString();
    const QString deviceId = req->value("device_id").toString();
    if (username.isEmpty() || password.isEmpty() || deviceId.isEmpty()) {
      return error(400, 1, "Missing fields");
    }

    if (m_usersByUsername.contains(username)) {
      return error(409, 4, "Username exists");
    }

    AccountState account;
    account.userId = QString("usr_%1").arg(m_userSeq++);
    account.username = username;
    account.passwordDerived = password;
    account.deviceId = deviceId;
    account.accessToken = QString("acc_%1").arg(account.userId);
    account.refreshToken = QString("ref_%1").arg(account.userId);

    m_usersByUsername.insert(username, account);
    m_userByAccessToken.insert(account.accessToken, account.userId);
    m_userByRefreshToken.insert(account.refreshToken, account.userId);

    return ok(QJsonObject{{"user_id", account.userId},
                          {"access_token", account.accessToken},
                          {"refresh_token", account.refreshToken},
                          {"device_status", "created"},
                          {"sync_key_version", 1}});
  }

  if (route == "/v1/login" && method == "POST") {
    const auto req = parseJson(body);
    if (!req.has_value()) {
      return error(400, 1, "Invalid JSON");
    }

    const QString username = req->value("username").toString();
    const QString password = req->value("password_derived_value").toString();
    const QString deviceId = req->value("device_id").toString();

    if (!m_usersByUsername.contains(username)) {
      return error(401, 2, "Unauthorized");
    }

    auto account = m_usersByUsername.value(username);
    if (account.passwordDerived != password) {
      return error(401, 2, "Unauthorized");
    }

    account.deviceId = deviceId;
    account.accessToken =
        QString("acc_%1_%2").arg(account.userId, QString::number(QDateTime::currentMSecsSinceEpoch()));
    account.refreshToken =
        QString("ref_%1_%2").arg(account.userId, QString::number(QDateTime::currentMSecsSinceEpoch()));
    m_usersByUsername.insert(username, account);
    m_userByAccessToken.insert(account.accessToken, account.userId);
    m_userByRefreshToken.insert(account.refreshToken, account.userId);

    return ok(QJsonObject{{"user_id", account.userId},
                          {"access_token", account.accessToken},
                          {"refresh_token", account.refreshToken},
                          {"device_status", "existing"},
                          {"sync_key_version", 1}});
  }

  if (route == "/v1/refresh" && method == "POST") {
    const auto req = parseJson(body);
    if (!req.has_value()) {
      return error(400, 1, "Invalid JSON");
    }

    const QString refreshToken = req->value("refresh_token").toString();
    if (!m_userByRefreshToken.contains(refreshToken)) {
      return error(401, 2, "Unauthorized");
    }

    const QString userId = m_userByRefreshToken.value(refreshToken);
    const QString access = QString("acc_%1_ref").arg(userId);
    const QString refresh = QString("ref_%1_ref").arg(userId);
    m_userByAccessToken.insert(access, userId);
    m_userByRefreshToken.insert(refresh, userId);

    return ok(QJsonObject{{"access_token", access}, {"refresh_token", refresh}});
  }

  if (route == "/v1/logout" && method == "POST") {
    return ok(QJsonObject{});
  }

  if (route == "/v1/me" && method == "GET") {
    const QString userId = authUserFromHeaders(headers);
    if (userId.isEmpty()) {
      return error(401, 2, "Unauthorized");
    }

    for (auto it = m_usersByUsername.begin(); it != m_usersByUsername.end(); ++it) {
      if (it->userId == userId) {
        return ok(QJsonObject{{"user_id", it->userId},
                              {"username", it->username},
                              {"current_device_id", it->deviceId},
                              {"sync_key_version", 1}});
      }
    }

    return error(404, 3, "Not found");
  }

  if (m_forceUnauthorized) {
    return error(401, 2, "Unauthorized");
  }

  const QString userId = authUserFromHeaders(headers);
  if (userId.isEmpty()) {
    return error(401, 2, "Unauthorized");
  }

  if (route == "/v1/conversations" && method == "GET") {
    QJsonArray conversations;
    conversations.push_back(QJsonObject{{"conversation_id", "conv_1"},
                                        {"type", 0},
                                        {"created_by", userId},
                                        {"created_by_username", "alice"},
                                        {"peer_user_id", "usr_peer"},
                                        {"created_at", 1710000000},
                                        {"membership_version", 1},
                                        {"my_role", "member"}});
    return ok(QJsonObject{{"conversations", conversations}});
  }

  if (route == "/v1/messages/send" && method == "POST") {
    if (m_failNextSend) {
      m_failNextSend = false;
      return error(503, 9, "Queue full");
    }

    const auto req = parseJson(body);
    if (!req.has_value()) {
      return error(400, 1, "Invalid JSON");
    }

    Envelope env;
    env.envelopeId = req->value("envelope_id").toString();
    env.conversationId = req->value("conversation_id").toString();
    env.senderUserId = userId;
    env.senderDeviceId = req->value("device_id").toString();
    env.ciphertext = req->value("ciphertext").toString();
    env.serverTimestamp = QDateTime::currentSecsSinceEpoch();
    env.envelopeType = req->value("envelope_type").toInt();

    if (env.envelopeId.isEmpty()) {
      env.envelopeId = QString("env_%1").arg(m_envelopeSeq++);
    }
    m_envelopes.push_back(env);

    return ok(QJsonObject{{"envelope_id", env.envelopeId},
                          {"server_timestamp", static_cast<double>(env.serverTimestamp)},
                          {"delivered_to_count", 1}});
  }

  if (route == "/v1/messages/ack" && method == "POST") {
    return ok(QJsonObject{});
  }

  if (route == "/v1/sync/pending" && method == "GET") {
    QJsonArray envelopes;
    for (const auto &env : m_envelopes) {
      envelopes.push_back(QJsonObject{{"envelope_id", env.envelopeId},
                                      {"conversation_id", env.conversationId},
                                      {"sender_user_id", env.senderUserId},
                                      {"sender_device_id", env.senderDeviceId},
                                      {"ciphertext", env.ciphertext},
                                      {"server_timestamp", static_cast<double>(env.serverTimestamp)},
                                      {"envelope_type", env.envelopeType}});
    }

    return ok(QJsonObject{{"envelopes", envelopes}, {"next_cursor", ""}, {"has_more", false}});
  }

  if (route == "/v1/attachments/upload-init" && method == "POST") {
    const QString id = QString("att_%1").arg(m_attachmentSeq++);
    m_attachments.insert(id, {});
    return ok(QJsonObject{{"attachment_id", id}, {"blob_path", "/blob/" + id}});
  }

  if (route.startsWith("/v1/attachments/") && route.endsWith("/chunk") && method == "PUT") {
    const QString attachmentId = route.mid(QString("/v1/attachments/").size());
    const QString id = attachmentId.left(attachmentId.indexOf("/chunk"));
    auto stored = m_attachments.value(id);

    const auto query = queryFromPath(path);
    const qint64 offset = query.queryItemValue("offset").toLongLong();
    if (offset < 0) {
      return error(400, 1, "Bad offset");
    }

    if (stored.size() < offset) {
      stored.append(QByteArray(static_cast<int>(offset - stored.size()), '\0'));
    }
    if (stored.size() == offset) {
      stored.append(body);
    } else {
      stored.replace(static_cast<int>(offset), body.size(), body);
    }

    m_attachments[id] = stored;
    return ok(QJsonObject{});
  }

  if (route.startsWith("/v1/attachments/") && route.endsWith("/finalize") && method == "POST") {
    return ok(QJsonObject{});
  }

  if (route.startsWith("/v1/attachments/") && method == "GET") {
    const QString id = route.mid(QString("/v1/attachments/").size());
    return {200, m_attachments.value(id), {}};
  }

  if (route.startsWith("/v1/users/by-username/") && method == "GET") {
    const QString username = route.mid(QString("/v1/users/by-username/").size());
    if (!m_usersByUsername.contains(username)) {
      return error(404, 3, "Not found");
    }
    const auto state = m_usersByUsername.value(username);
    return ok(QJsonObject{{"user_id", state.userId}, {"username", state.username}});
  }

  if (route == "/v1/users/search" && method == "GET") {
    QJsonArray users;
    for (auto it = m_usersByUsername.begin(); it != m_usersByUsername.end(); ++it) {
      users.push_back(QJsonObject{{"user_id", it->userId}, {"username", it->username}});
    }
    return ok(QJsonObject{{"users", users}});
  }

  if (route.startsWith("/v1/users/") && route.endsWith("/devices") && method == "GET") {
    const QString userIdInPath = route.mid(QString("/v1/users/").size());
    const QString userId = userIdInPath.left(userIdInPath.indexOf("/devices"));

    QJsonArray devices;
    for (auto it = m_usersByUsername.begin(); it != m_usersByUsername.end(); ++it) {
      if (it->userId == userId) {
        devices.push_back(QJsonObject{{"device_id", it->deviceId},
                                      {"device_label", "Desktop"},
                                      {"identity_key_public", "idpk"},
                                      {"signed_prekey_public", "spk"},
                                      {"signed_prekey_signature", "sig"}});
      }
    }
    return ok(QJsonObject{{"devices", devices}});
  }

  if (route == "/v1/sync/key-bundle" && method == "GET") {
    return ok(QJsonObject{{"version", 1},
                          {"wrapped_sync_key", "wrapped"},
                          {"sync_wrap_salt", "salt"},
                          {"sync_wrap_params", QJsonObject{{"algorithm", "argon2id"}}}});
  }

  if (route == "/v1/sync/key-bundle" && method == "PUT") {
    return ok(QJsonObject{});
  }

  if (route == "/v1/sync/changes" && method == "GET") {
    return ok(QJsonObject{{"records", QJsonArray{}}, {"next_cursor", ""}, {"has_more", false}});
  }

  if (route.startsWith("/v1/sync/records/") && (method == "PUT" || method == "DELETE")) {
    return ok(QJsonObject{});
  }

  return error(404, 3, "Not found");
}

void FakeVoxServer::setForceUnauthorized(bool value) {
  m_forceUnauthorized = value;
}

void FakeVoxServer::setFailNextSend(bool value) {
  m_failNextSend = value;
}

network::NetworkResponse FakeVoxServer::ok(const QJsonObject &obj, int status) const {
  return {status, QJsonDocument(obj).toJson(QJsonDocument::Compact), {}};
}

network::NetworkResponse FakeVoxServer::error(int status, int code, const QString &message) const {
  const QJsonObject body{{"error", QJsonObject{{"code", code}, {"message", message}}}};
  return {status, QJsonDocument(body).toJson(QJsonDocument::Compact), message};
}

std::optional<QJsonObject> FakeVoxServer::parseJson(QByteArrayView bytes) const {
  const auto doc = QJsonDocument::fromJson(bytes.toByteArray());
  if (!doc.isObject()) {
    return std::nullopt;
  }
  return doc.object();
}

QString FakeVoxServer::authUserFromHeaders(const network::NetworkAccess::HeaderMap &headers) const {
  if (m_forceUnauthorized) {
    return {};
  }

  const QString authorization = headers.value("Authorization");
  if (!authorization.startsWith("Bearer ")) {
    return {};
  }

  const QString token = authorization.mid(QString("Bearer ").size());
  return m_userByAccessToken.value(token);
}
