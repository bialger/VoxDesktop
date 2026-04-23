#include "storage/Repositories.hpp"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>

namespace vox::storage {
namespace {

bool exec(QSqlQuery &query) {
  return query.exec();
}

qint64 nowMs() {
  return QDateTime::currentMSecsSinceEpoch();
}

} // namespace

SqlAccountsRepository::SqlAccountsRepository(QSqlDatabase db) : m_db(std::move(db)) {
}

bool SqlAccountsRepository::upsertAccount(const domain::Account &account) {
  QSqlQuery query(m_db);
  query.prepare(
      "INSERT INTO accounts(account_id, server_id, user_id, username, active_device_id, "
      "refresh_token_ciphertext, last_login_ms) VALUES(?, ?, ?, ?, ?, ?, ?) "
      "ON CONFLICT(account_id) DO UPDATE SET server_id=excluded.server_id, "
      "user_id=excluded.user_id, username=excluded.username, "
      "active_device_id=excluded.active_device_id, "
      "refresh_token_ciphertext=excluded.refresh_token_ciphertext, "
      "last_login_ms=excluded.last_login_ms");
  query.addBindValue(account.accountId);
  query.addBindValue(account.serverId);
  query.addBindValue(account.userId);
  query.addBindValue(account.username);
  query.addBindValue(account.activeDeviceId);
  query.addBindValue(account.refreshTokenCiphertext);
  query.addBindValue(account.lastLogin.toMSecsSinceEpoch());
  return exec(query);
}

std::optional<domain::Account> SqlAccountsRepository::activeAccount() const {
  QSqlQuery query(m_db);
  if (!query.exec("SELECT account_id, server_id, user_id, username, active_device_id, "
                  "refresh_token_ciphertext, last_login_ms FROM accounts ORDER BY last_login_ms DESC LIMIT 1") ||
      !query.next()) {
    return std::nullopt;
  }

  domain::Account account;
  account.accountId = query.value(0).toString();
  account.serverId = query.value(1).toString();
  account.userId = query.value(2).toString();
  account.username = query.value(3).toString();
  account.activeDeviceId = query.value(4).toString();
  account.refreshTokenCiphertext = query.value(5).toByteArray();
  account.lastLogin = QDateTime::fromMSecsSinceEpoch(query.value(6).toLongLong());
  return account;
}

bool SqlAccountsRepository::deleteAccount(const QString &accountId) {
  if (accountId.isEmpty()) {
    return false;
  }
  QSqlQuery query(m_db);
  query.prepare("DELETE FROM accounts WHERE account_id = ?");
  query.addBindValue(accountId);
  return exec(query);
}

SqlConversationsRepository::SqlConversationsRepository(QSqlDatabase db) : m_db(std::move(db)) {
}

bool SqlConversationsRepository::upsertConversation(const domain::Conversation &conversation) {
  QSqlQuery query(m_db);
  query.prepare(
      "INSERT INTO conversations(conversation_id, server_id, type, title_ciphertext, "
      "last_message_at_ms, last_sort_key, mute_state, pin_rank, draft_ciphertext, epoch) "
      "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
      "ON CONFLICT(conversation_id) DO UPDATE SET "
      "server_id=excluded.server_id, type=excluded.type, title_ciphertext=excluded.title_ciphertext, "
      "last_message_at_ms=excluded.last_message_at_ms, last_sort_key=excluded.last_sort_key, "
      "mute_state=excluded.mute_state, pin_rank=excluded.pin_rank, draft_ciphertext=excluded.draft_ciphertext, "
      "epoch=excluded.epoch");
  query.addBindValue(conversation.conversationId);
  query.addBindValue(conversation.serverId);
  query.addBindValue(domain::toString(conversation.type));
  query.addBindValue(conversation.title.toUtf8());
  query.addBindValue(conversation.lastMessageAt.toMSecsSinceEpoch());
  query.addBindValue(conversation.lastMessageAt.toMSecsSinceEpoch());
  query.addBindValue(conversation.muted ? 1 : 0);
  query.addBindValue(conversation.pinRank);
  query.addBindValue(QByteArray{});
  query.addBindValue(conversation.epoch);
  return exec(query);
}

QVector<domain::Conversation> SqlConversationsRepository::listConversations() const {
  QVector<domain::Conversation> out;
  QSqlQuery query(m_db);
  if (!query.exec("SELECT conversation_id, server_id, type, title_ciphertext, last_message_at_ms, "
                  "mute_state, pin_rank, epoch FROM conversations ORDER BY pin_rank DESC, "
                  "last_message_at_ms DESC")) {
    return out;
  }

  while (query.next()) {
    domain::Conversation conversation;
    conversation.conversationId = query.value(0).toString();
    conversation.serverId = query.value(1).toString();
    conversation.type =
        domain::conversationTypeFromString(query.value(2).toString()).value_or(domain::ConversationType::Dm);
    conversation.title = QString::fromUtf8(query.value(3).toByteArray());
    conversation.lastMessageAt = QDateTime::fromMSecsSinceEpoch(query.value(4).toLongLong());
    conversation.muted = query.value(5).toInt() != 0;
    conversation.pinRank = query.value(6).toInt();
    conversation.epoch = query.value(7).toInt();
    out.push_back(std::move(conversation));
  }

  return out;
}

SqlMessagesRepository::SqlMessagesRepository(QSqlDatabase db) : m_db(std::move(db)) {
}

bool SqlMessagesRepository::upsertMessage(const domain::Message &message) {
  QSqlQuery query(m_db);
  query.prepare(
      "INSERT INTO messages(message_id, conversation_id, sender_user_id, sender_device_id, "
      "server_seq, client_created_at_ms, server_received_at_ms, content_kind, ciphertext_blob, "
      "plaintext_cache_ciphertext, delivery_state, read_state, is_outgoing, reply_to_message_id, "
      "sender_key_epoch) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
      "ON CONFLICT(message_id) DO UPDATE SET "
      "sender_user_id=excluded.sender_user_id, "
      "sender_device_id=excluded.sender_device_id, "
      "client_created_at_ms=excluded.client_created_at_ms, "
      "server_received_at_ms=excluded.server_received_at_ms, "
      "ciphertext_blob=excluded.ciphertext_blob, "
      "plaintext_cache_ciphertext=excluded.plaintext_cache_ciphertext, "
      "delivery_state=excluded.delivery_state, "
      "is_outgoing=excluded.is_outgoing");
  query.addBindValue(message.messageId);
  query.addBindValue(message.conversationId);
  query.addBindValue(message.senderUserId);
  query.addBindValue(message.senderDeviceId);
  query.addBindValue(message.serverSeq);
  query.addBindValue(message.clientCreatedAtMs);
  query.addBindValue(message.serverReceivedAtMs);
  query.addBindValue(domain::toString(message.contentKind));
  query.addBindValue(message.ciphertextBlob);
  query.addBindValue(message.plaintextCacheCiphertext);
  query.addBindValue(domain::toString(message.deliveryState));
  query.addBindValue("unread");
  query.addBindValue(message.isOutgoing ? 1 : 0);
  query.addBindValue(message.replyToMessageId.value_or(QString{}));
  query.addBindValue(message.senderKeyEpoch);
  return exec(query);
}

QVector<domain::Message> SqlMessagesRepository::listConversationMessages(const QString &conversationId,
                                                                         int limit) const {
  QVector<domain::Message> out;
  QSqlQuery query(m_db);
  query.prepare(
      "SELECT message_id, conversation_id, sender_user_id, sender_device_id, server_seq, "
      "client_created_at_ms, server_received_at_ms, content_kind, ciphertext_blob, "
      "plaintext_cache_ciphertext, delivery_state, is_outgoing, reply_to_message_id, sender_key_epoch "
      "FROM messages WHERE conversation_id = ? ORDER BY client_created_at_ms ASC LIMIT ?");
  query.addBindValue(conversationId);
  query.addBindValue(limit);

  if (!exec(query)) {
    return out;
  }

  while (query.next()) {
    domain::Message message;
    message.messageId = query.value(0).toString();
    message.conversationId = query.value(1).toString();
    message.senderUserId = query.value(2).toString();
    message.senderDeviceId = query.value(3).toString();
    message.serverSeq = query.value(4).toLongLong();
    message.clientCreatedAtMs = query.value(5).toLongLong();
    message.serverReceivedAtMs = query.value(6).toLongLong();
    message.contentKind = domain::contentKindFromString(query.value(7).toString()).value_or(domain::ContentKind::Text);
    message.ciphertextBlob = query.value(8).toByteArray();
    message.plaintextCacheCiphertext = query.value(9).toByteArray();
    message.deliveryState =
        domain::deliveryStateFromString(query.value(10).toString()).value_or(domain::DeliveryState::Queued);
    message.isOutgoing = query.value(11).toInt() != 0;
    if (!query.value(12).toString().isEmpty()) {
      message.replyToMessageId = query.value(12).toString();
    }
    message.senderKeyEpoch = query.value(13).toInt();
    out.push_back(std::move(message));
  }

  return out;
}

SqlDevicesRepository::SqlDevicesRepository(QSqlDatabase db) : m_db(std::move(db)) {
}

bool SqlDevicesRepository::upsertDevice(const domain::Device &device) {
  QSqlQuery query(m_db);
  query.prepare(
      "INSERT INTO devices(device_id, user_id, server_id, label, identity_sign_pub, identity_dh_pub, "
      "signed_prekey_pub, signed_prekey_id, signed_prekey_sig, last_seen_ms, trust_state) "
      "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
      "ON CONFLICT(device_id) DO UPDATE SET label=excluded.label, last_seen_ms=excluded.last_seen_ms, "
      "trust_state=excluded.trust_state");
  query.addBindValue(device.deviceId);
  query.addBindValue(device.userId);
  query.addBindValue(device.serverId);
  query.addBindValue(device.label);
  query.addBindValue(device.identitySignPub);
  query.addBindValue(device.identityDhPub);
  query.addBindValue(QByteArray{});
  query.addBindValue(0);
  query.addBindValue(QByteArray{});
  query.addBindValue(device.lastSeen.toMSecsSinceEpoch());
  query.addBindValue(domain::toString(device.trustLevel));
  return exec(query);
}

QVector<domain::Device> SqlDevicesRepository::listDevicesByUser(const QString &userId) const {
  QVector<domain::Device> out;
  QSqlQuery query(m_db);
  query.prepare(
      "SELECT device_id, user_id, server_id, label, identity_sign_pub, identity_dh_pub, "
      "last_seen_ms, trust_state FROM devices WHERE user_id = ? ORDER BY last_seen_ms DESC");
  query.addBindValue(userId);

  if (!exec(query)) {
    return out;
  }

  while (query.next()) {
    domain::Device device;
    device.deviceId = query.value(0).toString();
    device.userId = query.value(1).toString();
    device.serverId = query.value(2).toString();
    device.label = query.value(3).toString();
    device.identitySignPub = query.value(4).toByteArray();
    device.identityDhPub = query.value(5).toByteArray();
    device.lastSeen = QDateTime::fromMSecsSinceEpoch(query.value(6).toLongLong());
    device.trustLevel = domain::trustLevelFromString(query.value(7).toString()).value_or(domain::TrustLevel::Unknown);
    out.push_back(std::move(device));
  }

  return out;
}

SqlSyncRepository::SqlSyncRepository(QSqlDatabase db) : m_db(std::move(db)) {
}

bool SqlSyncRepository::upsertRecord(const domain::SyncRecord &record) {
  QSqlQuery query(m_db);
  query.prepare(
      "INSERT INTO sync_records(collection, record_id, version, ciphertext, updated_at_ms, tombstone) "
      "VALUES(?, ?, ?, ?, ?, ?) "
      "ON CONFLICT(collection, record_id) DO UPDATE SET version=excluded.version, "
      "ciphertext=excluded.ciphertext, updated_at_ms=excluded.updated_at_ms, tombstone=excluded.tombstone");
  query.addBindValue(record.collection);
  query.addBindValue(record.recordId);
  query.addBindValue(record.version);
  query.addBindValue(record.ciphertext);
  query.addBindValue(record.updatedAtMs);
  query.addBindValue(record.tombstone ? 1 : 0);
  return exec(query);
}

QVector<domain::SyncRecord> SqlSyncRepository::listRecords(const QString &collection) const {
  QVector<domain::SyncRecord> out;
  QSqlQuery query(m_db);
  query.prepare(
      "SELECT collection, record_id, version, ciphertext, updated_at_ms, tombstone "
      "FROM sync_records WHERE collection = ? ORDER BY updated_at_ms DESC");
  query.addBindValue(collection);

  if (!exec(query)) {
    return out;
  }

  while (query.next()) {
    domain::SyncRecord record;
    record.collection = query.value(0).toString();
    record.recordId = query.value(1).toString();
    record.version = query.value(2).toInt();
    record.ciphertext = query.value(3).toByteArray();
    record.updatedAtMs = query.value(4).toLongLong();
    record.tombstone = query.value(5).toInt() != 0;
    out.push_back(std::move(record));
  }

  return out;
}

SqlJobsRepository::SqlJobsRepository(QSqlDatabase db) : m_db(std::move(db)) {
}

bool SqlJobsRepository::enqueueJob(const QString &type, QByteArray payloadCiphertext, qint64 nextAttemptMs) {
  QSqlQuery query(m_db);
  query.prepare(
      "INSERT INTO jobs(type, payload_ciphertext, state, attempt_count, next_attempt_ms) "
      "VALUES(?, ?, 'queued', 0, ?)");
  query.addBindValue(type);
  query.addBindValue(std::move(payloadCiphertext));
  query.addBindValue(nextAttemptMs);
  return exec(query);
}

QVector<JobRecord> SqlJobsRepository::dueJobs(qint64 nowMs) const {
  QVector<JobRecord> out;
  QSqlQuery query(m_db);
  query.prepare(
      "SELECT job_id, type, payload_ciphertext, state, attempt_count, next_attempt_ms "
      "FROM jobs WHERE next_attempt_ms <= ? AND state != 'done' ORDER BY next_attempt_ms ASC");
  query.addBindValue(nowMs);

  if (!exec(query)) {
    return out;
  }

  while (query.next()) {
    JobRecord job;
    job.jobId = query.value(0).toLongLong();
    job.type = query.value(1).toString();
    job.payloadCiphertext = query.value(2).toByteArray();
    job.state = query.value(3).toString();
    job.attemptCount = query.value(4).toInt();
    job.nextAttemptMs = query.value(5).toLongLong();
    out.push_back(std::move(job));
  }

  return out;
}

bool SqlJobsRepository::markAttempt(qint64 jobId, bool success, qint64 nextAttemptMs) {
  QSqlQuery query(m_db);
  query.prepare("UPDATE jobs SET state = ?, attempt_count = attempt_count + 1, next_attempt_ms = ? WHERE job_id = ?");
  query.addBindValue(success ? "done" : "queued");
  query.addBindValue(nextAttemptMs);
  query.addBindValue(jobId);
  return exec(query);
}

SqlSearchRepository::SqlSearchRepository(QSqlDatabase db) : m_db(std::move(db)) {
}

bool SqlSearchRepository::insertToken(const QString &conversationId,
                                      const QString &messageId,
                                      QByteArray tokenDigest,
                                      int positionHint) {
  QSqlQuery query(m_db);
  query.prepare(
      "INSERT INTO search_tokens(conversation_id, message_id, token_digest, position_hint) "
      "VALUES(?, ?, ?, ?)");
  query.addBindValue(conversationId);
  query.addBindValue(messageId);
  query.addBindValue(std::move(tokenDigest));
  query.addBindValue(positionHint);
  return exec(query);
}

QVector<QString> SqlSearchRepository::findMessagesByToken(const QByteArray &tokenDigest) const {
  QVector<QString> out;
  QSqlQuery query(m_db);
  query.prepare("SELECT message_id FROM search_tokens WHERE token_digest = ?");
  query.addBindValue(tokenDigest);

  if (!exec(query)) {
    return out;
  }

  while (query.next()) {
    out.push_back(query.value(0).toString());
  }
  return out;
}

SqlTrustRepository::SqlTrustRepository(QSqlDatabase db) : m_db(std::move(db)) {
}

bool SqlTrustRepository::upsertTrust(const domain::DeviceFingerprint &trust) {
  QSqlQuery query(m_db);
  query.prepare(
      "INSERT INTO trusted_devices(device_id, fingerprint, trust_level, verified_at_ms, note_ciphertext) "
      "VALUES(?, ?, ?, ?, ?) "
      "ON CONFLICT(device_id) DO UPDATE SET fingerprint=excluded.fingerprint, "
      "trust_level=excluded.trust_level, verified_at_ms=excluded.verified_at_ms");
  query.addBindValue(trust.deviceId);
  query.addBindValue(trust.fingerprint);
  query.addBindValue(domain::toString(trust.trustLevel));
  query.addBindValue(trust.verifiedAtMs > 0 ? trust.verifiedAtMs : nowMs());
  query.addBindValue(QByteArray{});
  return exec(query);
}

std::optional<domain::DeviceFingerprint> SqlTrustRepository::findTrust(const QString &deviceId) const {
  QSqlQuery query(m_db);
  query.prepare("SELECT device_id, fingerprint, trust_level, verified_at_ms FROM trusted_devices WHERE device_id = ?");
  query.addBindValue(deviceId);
  if (!exec(query) || !query.next()) {
    return std::nullopt;
  }

  domain::DeviceFingerprint trust;
  trust.deviceId = query.value(0).toString();
  trust.fingerprint = query.value(1).toString();
  trust.trustLevel = domain::trustLevelFromString(query.value(2).toString()).value_or(domain::TrustLevel::Unknown);
  trust.verifiedAtMs = query.value(3).toLongLong();
  return trust;
}

} // namespace vox::storage
