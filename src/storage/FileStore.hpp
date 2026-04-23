#ifndef VOX_STORAGE_FILESTORE_HPP
#define VOX_STORAGE_FILESTORE_HPP

#include <QByteArray>
#include <QString>
#include <optional>

namespace vox::storage {

class FileStore final {
public:
    explicit FileStore(QString rootPath);

    bool initialize();

    QString encryptedAttachmentPath(const QString &attachmentId) const;
    QString decryptedPreviewPath(const QString &attachmentId) const;

    bool writeEncryptedAttachment(const QString &attachmentId, QByteArrayView bytes);
    std::optional<QByteArray> readEncryptedAttachment(const QString &attachmentId) const;

    bool writeDecryptedPreview(const QString &attachmentId, QByteArrayView bytes);
    bool removeDecryptedPreview(const QString &attachmentId);

private:
    QString m_rootPath;
};

} // namespace vox::storage

#endif // VOX_STORAGE_FILESTORE_HPP
