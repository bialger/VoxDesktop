#include "storage/Migrations.hpp"

#include <QSqlError>
#include <QSqlQuery>

namespace vox::storage {
namespace {

bool exec(QSqlQuery &query, const QString &statement, QString *error) {
  if (query.exec(statement)) {
    return true;
  }
  if (error != nullptr) {
    *error = query.lastError().text();
  }
  return false;
}

} // namespace

bool Migrations::run(QSqlDatabase db, QString *error) {
  QSqlQuery query(db);

  if (!exec(query,
            "CREATE TABLE IF NOT EXISTS schema_migrations ("
            "id INTEGER PRIMARY KEY CHECK(id = 1),"
            "version INTEGER NOT NULL"
            ")",
            error)) {
    return false;
  }

  if (!exec(query, "INSERT OR IGNORE INTO schema_migrations(id, version) VALUES(1, 0)", error)) {
    return false;
  }

  int version = 0;
  if (!exec(query, "SELECT version FROM schema_migrations WHERE id = 1", error) || !query.next()) {
    return false;
  }
  version = query.value(0).toInt();

  if (version >= currentVersion()) {
    return true;
  }

  if (!db.transaction()) {
    if (error != nullptr) {
      *error = db.lastError().text();
    }
    return false;
  }

  const QStringList statements{
      "CREATE TABLE IF NOT EXISTS servers ("
      "server_id TEXT PRIMARY KEY,"
      "base_url TEXT NOT NULL,"
      "display_name TEXT NOT NULL,"
      "tls_mode TEXT NOT NULL,"
      "pinned_cert_sha256 TEXT,"
      "created_at_ms INTEGER NOT NULL"
      ")",
      "CREATE TABLE IF NOT EXISTS accounts ("
      "account_id TEXT PRIMARY KEY,"
      "server_id TEXT NOT NULL,"
      "user_id TEXT NOT NULL,"
      "username TEXT NOT NULL,"
      "active_device_id TEXT NOT NULL,"
      "refresh_token_ciphertext BLOB NOT NULL,"
      "last_login_ms INTEGER NOT NULL"
      ")",
      "CREATE TABLE IF NOT EXISTS devices ("
      "device_id TEXT PRIMARY KEY,"
      "user_id TEXT NOT NULL,"
      "server_id TEXT NOT NULL,"
      "label TEXT NOT NULL,"
      "identity_sign_pub BLOB,"
      "identity_dh_pub BLOB,"
      "signed_prekey_pub BLOB,"
      "signed_prekey_id INTEGER,"
      "signed_prekey_sig BLOB,"
      "last_seen_ms INTEGER NOT NULL,"
      "trust_state TEXT NOT NULL"
      ")",
      "CREATE TABLE IF NOT EXISTS conversations ("
      "conversation_id TEXT PRIMARY KEY,"
      "server_id TEXT NOT NULL,"
      "type TEXT NOT NULL,"
      "title_ciphertext BLOB,"
      "last_message_at_ms INTEGER NOT NULL,"
      "last_sort_key INTEGER NOT NULL,"
      "mute_state INTEGER NOT NULL,"
      "pin_rank INTEGER NOT NULL,"
      "draft_ciphertext BLOB,"
      "epoch INTEGER NOT NULL"
      ")",
      "CREATE TABLE IF NOT EXISTS messages ("
      "message_id TEXT PRIMARY KEY,"
      "conversation_id TEXT NOT NULL,"
      "sender_user_id TEXT NOT NULL,"
      "sender_device_id TEXT NOT NULL,"
      "server_seq INTEGER NOT NULL,"
      "client_created_at_ms INTEGER NOT NULL,"
      "server_received_at_ms INTEGER NOT NULL,"
      "content_kind TEXT NOT NULL,"
      "ciphertext_blob BLOB NOT NULL,"
      "plaintext_cache_ciphertext BLOB,"
      "delivery_state TEXT NOT NULL,"
      "read_state TEXT NOT NULL,"
      "is_outgoing INTEGER NOT NULL,"
      "reply_to_message_id TEXT,"
      "sender_key_epoch INTEGER NOT NULL"
      ")",
      "CREATE TABLE IF NOT EXISTS attachments ("
      "attachment_id TEXT PRIMARY KEY,"
      "message_id TEXT NOT NULL,"
      "ciphertext_path TEXT NOT NULL,"
      "mime_type_ciphertext BLOB,"
      "file_name_ciphertext BLOB,"
      "plaintext_size INTEGER NOT NULL,"
      "ciphertext_size INTEGER NOT NULL,"
      "sha256_ciphertext BLOB,"
      "download_state TEXT NOT NULL"
      ")",
      "CREATE TABLE IF NOT EXISTS sessions ("
      "peer_device_id TEXT PRIMARY KEY,"
      "session_version INTEGER NOT NULL,"
      "state_blob_ciphertext BLOB NOT NULL,"
      "updated_at_ms INTEGER NOT NULL"
      ")",
      "CREATE TABLE IF NOT EXISTS sender_keys ("
      "conversation_id TEXT NOT NULL,"
      "owner_device_id TEXT NOT NULL,"
      "epoch INTEGER NOT NULL,"
      "state_blob_ciphertext BLOB NOT NULL,"
      "updated_at_ms INTEGER NOT NULL,"
      "PRIMARY KEY (conversation_id, owner_device_id, epoch)"
      ")",
      "CREATE TABLE IF NOT EXISTS sync_records ("
      "collection TEXT NOT NULL,"
      "record_id TEXT NOT NULL,"
      "version INTEGER NOT NULL,"
      "ciphertext BLOB NOT NULL,"
      "updated_at_ms INTEGER NOT NULL,"
      "tombstone INTEGER NOT NULL DEFAULT 0,"
      "PRIMARY KEY (collection, record_id)"
      ")",
      "CREATE TABLE IF NOT EXISTS trusted_devices ("
      "device_id TEXT PRIMARY KEY,"
      "fingerprint TEXT NOT NULL,"
      "trust_level TEXT NOT NULL,"
      "verified_at_ms INTEGER NOT NULL,"
      "note_ciphertext BLOB"
      ")",
      "CREATE TABLE IF NOT EXISTS search_tokens ("
      "conversation_id TEXT NOT NULL,"
      "message_id TEXT NOT NULL,"
      "token_digest BLOB NOT NULL,"
      "position_hint INTEGER NOT NULL"
      ")",
      "CREATE TABLE IF NOT EXISTS jobs ("
      "job_id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "type TEXT NOT NULL,"
      "payload_ciphertext BLOB NOT NULL,"
      "state TEXT NOT NULL,"
      "attempt_count INTEGER NOT NULL DEFAULT 0,"
      "next_attempt_ms INTEGER NOT NULL"
      ")",
      "UPDATE schema_migrations SET version = 1 WHERE id = 1"};

  for (const auto &statement : statements) {
    if (!exec(query, statement, error)) {
      db.rollback();
      return false;
    }
  }

  return db.commit();
}

int Migrations::currentVersion() {
  return 1;
}

} // namespace vox::storage
