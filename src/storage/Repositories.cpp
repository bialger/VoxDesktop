#include "storage/Repositories.hpp"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>

#include <cstdint>

namespace vox::storage {
namespace {

bool Exec(QSqlQuery &query) {
  return query.exec();
}

qint64 NowMs() {
  return QDateTime::currentMSecsSinceEpoch();
}

enum AccountColumn : std::uint8_t {
  kAccountId = 0,
  kServerId = 1,
  kUserId = 2,
  kUsername = 3,
  kActiveDeviceId = 4,
  kRefreshTokenCiphertext = 5,
  kLastLoginMs = 6,
};

enum ConversationColumn : std::uint8_t {
  kConversationId = 0,
  kConversationServerId = 1,
  kConversationType = 2,
  kTitleCiphertext = 3,
  kLastMessageAtMs = 4,
  kMuteState = 5,
  kPinRank = 6,
  kEpoch = 7,
};

enum MessageColumn : std::uint8_t {
  kMessageId = 0,
  kMessageConversationId = 1,
  kSenderUserId = 2,
  kSenderDeviceId = 3,
  kServerSeq = 4,
  kClientCreatedAtMs = 5,
  kServerReceivedAtMs = 6,
  kContentKind = 7,
  kCiphertextBlob = 8,
  kPlaintextCacheCiphertext = 9,
  kDeliveryState = 10,
  kIsOutgoing = 11,
  kReplyToMessageId = 12,
  kSenderKeyEpoch = 13,
};

enum DeviceColumn : std::uint8_t {
  kDeviceId = 0,
  kDeviceUserId = 1,
  kDeviceServerId = 2,
  kDeviceLabel = 3,
  kIdentitySignPub = 4,
  kIdentityDhPub = 5,
  kLastSeenMs = 6,
  kTrustState = 7,
};

enum SyncRecordColumn : std::uint8_t {
  kSyncCollection = 0,
  kSyncRecordId = 1,
  kSyncVersion = 2,
  kSyncCiphertext = 3,
  kSyncUpdatedAtMs = 4,
  kSyncTombstone = 5,
};

enum JobColumn : std::uint8_t {
  kJobId = 0,
  kJobType = 1,
  kJobPayloadCiphertext = 2,
  kJobState = 3,
  kJobAttemptCount = 4,
  kJobNextAttemptMs = 5,
};

} // namespace

SqlAccountsRepository::SqlAccountsRepository(const QSqlDatabase &db) : m_db(db) {
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
  return Exec(query);
}

std::optional<domain::Account> SqlAccountsRepository::activeAccount() const {
  QSqlQuery query(m_db);
  if (!query.exec("SELECT account_id, server_id, user_id, username, active_device_id, "
                  "refresh_token_ciphertext, last_login_ms FROM accounts ORDER BY last_login_ms DESC LIMIT 1") ||
      !query.next()) {
    return std::nullopt;
  }

  domain::Account account;
  account.accountId = query.value(kAccountId).toString();
  account.serverId = query.value(kServerId).toString();
  account.userId = query.value(kUserId).toString();
  account.username = query.value(kUsername).toString();
  account.activeDeviceId = query.value(kActiveDeviceId).toString();
  account.refreshTokenCiphertext = query.value(kRefreshTokenCiphertext).toByteArray();
  account.lastLogin = QDateTime::fromMSecsSinceEpoch(query.value(kLastLoginMs).toLongLong());
  return account;
}

bool SqlAccountsRepository::deleteAccount(const QString &accountId) {
  if (accountId.isEmpty()) {
    return false;
  }
  QSqlQuery query(m_db);
  query.prepare("DELETE FROM accounts WHERE account_id = ?");
  query.addBindValue(accountId);
  return Exec(query);
}

SqlConversationsRepository::SqlConversationsRepository(const QSqlDatabase &db) : m_db(db) {
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
  return Exec(query);
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
    conversation.conversationId = query.value(kConversationId).toString();
    conversation.serverId = query.value(kConversationServerId).toString();
    conversation.type = domain::conversationTypeFromString(query.value(kConversationType).toString())
                            .value_or(domain::ConversationType::Dm);
    conversation.title = QString::fromUtf8(query.value(kTitleCiphertext).toByteArray());
    conversation.lastMessageAt = QDateTime::fromMSecsSinceEpoch(query.value(kLastMessageAtMs).toLongLong());
    conversation.muted = query.value(kMuteState).toInt() != 0;
    conversation.pinRank = query.value(kPinRank).toInt();
    conversation.epoch = query.value(kEpoch).toInt();
    out.push_back(std::move(conversation));
  }

  return out;
}

SqlMessagesRepository::SqlMessagesRepository(const QSqlDatabase &db) : m_db(db) {
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
  return Exec(query);
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

  if (!Exec(query)) {
    return out;
  }

  while (query.next()) {
    domain::Message message;
    message.messageId = query.value(kMessageId).toString();
    message.conversationId = query.value(kMessageConversationId).toString();
    message.senderUserId = query.value(kSenderUserId).toString();
    message.senderDeviceId = query.value(kSenderDeviceId).toString();
    message.serverSeq = query.value(kServerSeq).toLongLong();
    message.clientCreatedAtMs = query.value(kClientCreatedAtMs).toLongLong();
    message.serverReceivedAtMs = query.value(kServerReceivedAtMs).toLongLong();
    message.contentKind =
        domain::contentKindFromString(query.value(kContentKind).toString()).value_or(domain::ContentKind::Text);
    message.ciphertextBlob = query.value(kCiphertextBlob).toByteArray();
    message.plaintextCacheCiphertext = query.value(kPlaintextCacheCiphertext).toByteArray();
    message.deliveryState =
        domain::deliveryStateFromString(query.value(kDeliveryState).toString()).value_or(domain::DeliveryState::Queued);
    message.isOutgoing = query.value(kIsOutgoing).toInt() != 0;
    if (!query.value(kReplyToMessageId).toString().isEmpty()) {
      message.replyToMessageId = query.value(kReplyToMessageId).toString();
    }
    message.senderKeyEpoch = query.value(kSenderKeyEpoch).toInt();
    out.push_back(std::move(message));
  }

  return out;
}

SqlDevicesRepository::SqlDevicesRepository(const QSqlDatabase &db) : m_db(db) {
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
  return Exec(query);
}

QVector<domain::Device> SqlDevicesRepository::listDevicesByUser(const QString &userId) const {
  QVector<domain::Device> out;
  QSqlQuery query(m_db);
  query.prepare(
      "SELECT device_id, user_id, server_id, label, identity_sign_pub, identity_dh_pub, "
      "last_seen_ms, trust_state FROM devices WHERE user_id = ? ORDER BY last_seen_ms DESC");
  query.addBindValue(userId);

  if (!Exec(query)) {
    return out;
  }

  while (query.next()) {
    domain::Device device;
    device.deviceId = query.value(kDeviceId).toString();
    device.userId = query.value(kDeviceUserId).toString();
    device.serverId = query.value(kDeviceServerId).toString();
    device.label = query.value(kDeviceLabel).toString();
    device.identitySignPub = query.value(kIdentitySignPub).toByteArray();
    device.identityDhPub = query.value(kIdentityDhPub).toByteArray();
    device.lastSeen = QDateTime::fromMSecsSinceEpoch(query.value(kLastSeenMs).toLongLong());
    device.trustLevel =
        domain::trustLevelFromString(query.value(kTrustState).toString()).value_or(domain::TrustLevel::Unknown);
    out.push_back(std::move(device));
  }

  return out;
}

SqlSyncRepository::SqlSyncRepository(const QSqlDatabase &db) : m_db(db) {
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
  return Exec(query);
}

QVector<domain::SyncRecord> SqlSyncRepository::listRecords(const QString &collection) const {
  QVector<domain::SyncRecord> out;
  QSqlQuery query(m_db);
  query.prepare(
      "SELECT collection, record_id, version, ciphertext, updated_at_ms, tombstone "
      "FROM sync_records WHERE collection = ? ORDER BY updated_at_ms DESC");
  query.addBindValue(collection);

  if (!Exec(query)) {
    return out;
  }

  while (query.next()) {
    domain::SyncRecord record;
    record.collection = query.value(kSyncCollection).toString();
    record.recordId = query.value(kSyncRecordId).toString();
    record.version = query.value(kSyncVersion).toInt();
    record.ciphertext = query.value(kSyncCiphertext).toByteArray();
    record.updatedAtMs = query.value(kSyncUpdatedAtMs).toLongLong();
    record.tombstone = query.value(kSyncTombstone).toInt() != 0;
    out.push_back(std::move(record));
  }

  return out;
}

SqlJobsRepository::SqlJobsRepository(const QSqlDatabase &db) : m_db(db) {
}

bool SqlJobsRepository::enqueueJob(const QString &type, QByteArray payloadCiphertext, qint64 nextAttemptMs) {
  QSqlQuery query(m_db);
  query.prepare(
      "INSERT INTO jobs(type, payload_ciphertext, state, attempt_count, next_attempt_ms) "
      "VALUES(?, ?, 'queued', 0, ?)");
  query.addBindValue(type);
  query.addBindValue(payloadCiphertext);
  query.addBindValue(nextAttemptMs);
  return Exec(query);
}

QVector<JobRecord> SqlJobsRepository::dueJobs(qint64 nowMs) const {
  QVector<JobRecord> out;
  QSqlQuery query(m_db);
  query.prepare(
      "SELECT job_id, type, payload_ciphertext, state, attempt_count, next_attempt_ms "
      "FROM jobs WHERE next_attempt_ms <= ? AND state != 'done' ORDER BY next_attempt_ms ASC");
  query.addBindValue(nowMs);

  if (!Exec(query)) {
    return out;
  }

  while (query.next()) {
    JobRecord job;
    job.jobId = query.value(kJobId).toLongLong();
    job.type = query.value(kJobType).toString();
    job.payloadCiphertext = query.value(kJobPayloadCiphertext).toByteArray();
    job.state = query.value(kJobState).toString();
    job.attemptCount = query.value(kJobAttemptCount).toInt();
    job.nextAttemptMs = query.value(kJobNextAttemptMs).toLongLong();
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
  return Exec(query);
}

SqlSearchRepository::SqlSearchRepository(const QSqlDatabase &db) : m_db(db) {
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
  query.addBindValue(tokenDigest);
  query.addBindValue(positionHint);
  return Exec(query);
}

QVector<QString> SqlSearchRepository::findMessagesByToken(const QByteArray &tokenDigest) const {
  QVector<QString> out;
  QSqlQuery query(m_db);
  query.prepare("SELECT message_id FROM search_tokens WHERE token_digest = ?");
  query.addBindValue(tokenDigest);

  if (!Exec(query)) {
    return out;
  }

  while (query.next()) {
    out.push_back(query.value(0).toString());
  }
  return out;
}

SqlTrustRepository::SqlTrustRepository(const QSqlDatabase &db) : m_db(db) {
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
  query.addBindValue(trust.verifiedAtMs > 0 ? trust.verifiedAtMs : NowMs());
  query.addBindValue(QByteArray{});
  return Exec(query);
}

std::optional<domain::DeviceFingerprint> SqlTrustRepository::findTrust(const QString &deviceId) const {
  QSqlQuery query(m_db);
  query.prepare("SELECT device_id, fingerprint, trust_level, verified_at_ms FROM trusted_devices WHERE device_id = ?");
  query.addBindValue(deviceId);
  if (!Exec(query) || !query.next()) {
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
