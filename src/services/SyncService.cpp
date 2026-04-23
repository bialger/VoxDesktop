#include "services/SyncService.hpp"

#include <QDateTime>

namespace vox::services {

SyncService::SyncService(network::SyncApi &api, storage::ISyncRepository &repository) :
    m_api(api), m_repository(repository) {
}

bool SyncService::pullCollection(const QString &collection) {
  QString cursor;
  do {
    const auto changes = m_api.changes(collection, cursor, 100);
    if (!changes.ok || !changes.data.has_value()) {
      return false;
    }

    for (const auto &record : changes.data->records) {
      domain::SyncRecord local;
      local.collection = record.collection;
      local.recordId = record.recordId;
      local.version = record.version;
      local.ciphertext = record.ciphertext.toUtf8();
      local.updatedAtMs = QDateTime::currentMSecsSinceEpoch();
      local.tombstone = record.deleted;
      if (!m_repository.upsertRecord(local)) {
        return false;
      }
    }

    cursor = changes.data->nextCursor;
    if (!changes.data->hasMore) {
      break;
    }
  } while (true);

  return true;
}

bool SyncService::pushRecord(const domain::SyncRecord &record) {
  network::SyncRecordDto dto;
  dto.collection = record.collection;
  dto.recordId = record.recordId;
  dto.version = record.version;
  dto.ciphertext = QString::fromUtf8(record.ciphertext);
  dto.deleted = record.tombstone;

  return m_api.putRecord(dto).ok;
}

} // namespace vox::services
