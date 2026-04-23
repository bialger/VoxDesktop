#include "storage/FileStore.hpp"

#include <QDir>
#include <QFile>
#include <QSaveFile>

namespace vox::storage {

FileStore::FileStore(QString rootPath) : m_rootPath(std::move(rootPath)) {
}

bool FileStore::initialize() {
  QDir root(m_rootPath);
  if (!root.exists() && !root.mkpath(".")) {
    return false;
  }

  return root.mkpath("encrypted") && root.mkpath("preview");
}

QString FileStore::encryptedAttachmentPath(const QString &attachmentId) const {
  return m_rootPath + "/encrypted/" + attachmentId + ".bin";
}

QString FileStore::decryptedPreviewPath(const QString &attachmentId) const {
  return m_rootPath + "/preview/" + attachmentId + ".bin";
}

bool FileStore::writeEncryptedAttachment(const QString &attachmentId, QByteArrayView bytes) {
  QSaveFile file(encryptedAttachmentPath(attachmentId));
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }

  file.write(bytes.data(), bytes.size());
  return file.commit();
}

std::optional<QByteArray> FileStore::readEncryptedAttachment(const QString &attachmentId) const {
  QFile file(encryptedAttachmentPath(attachmentId));
  if (!file.open(QIODevice::ReadOnly)) {
    return std::nullopt;
  }
  return file.readAll();
}

bool FileStore::writeDecryptedPreview(const QString &attachmentId, QByteArrayView bytes) {
  QSaveFile file(decryptedPreviewPath(attachmentId));
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }

  file.write(bytes.data(), bytes.size());
  return file.commit();
}

bool FileStore::removeDecryptedPreview(const QString &attachmentId) {
  return QFile::remove(decryptedPreviewPath(attachmentId));
}

} // namespace vox::storage
