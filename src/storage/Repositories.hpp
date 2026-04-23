#ifndef VOX_STORAGE_REPOSITORIES_HPP
#define VOX_STORAGE_REPOSITORIES_HPP

#include <QByteArray>
#include <QSqlDatabase>
#include <QString>
#include <QVector>
#include <optional>

#include "domain/Models.hpp"

namespace vox::storage {

struct JobRecord {
    qint64 jobId{0};
    QString type;
    QByteArray payloadCiphertext;
    QString state;
    int attemptCount{0};
    qint64 nextAttemptMs{0};
};

class IAccountsRepository {
public:
    virtual ~IAccountsRepository() = default;
    virtual bool upsertAccount(const domain::Account &account) = 0;
    virtual std::optional<domain::Account> activeAccount() const = 0;
    virtual bool deleteAccount(const QString &accountId) = 0;
};

class IConversationsRepository {
public:
    virtual ~IConversationsRepository() = default;
    virtual bool upsertConversation(const domain::Conversation &conversation) = 0;
    virtual QVector<domain::Conversation> listConversations() const = 0;
};

class IMessagesRepository {
public:
    virtual ~IMessagesRepository() = default;
    virtual bool upsertMessage(const domain::Message &message) = 0;
    virtual QVector<domain::Message> listConversationMessages(const QString &conversationId,
                                                               int limit) const = 0;
};

class IDevicesRepository {
public:
    virtual ~IDevicesRepository() = default;
    virtual bool upsertDevice(const domain::Device &device) = 0;
    virtual QVector<domain::Device> listDevicesByUser(const QString &userId) const = 0;
};

class ISyncRepository {
public:
    virtual ~ISyncRepository() = default;
    virtual bool upsertRecord(const domain::SyncRecord &record) = 0;
    virtual QVector<domain::SyncRecord> listRecords(const QString &collection) const = 0;
};

class IJobsRepository {
public:
    virtual ~IJobsRepository() = default;
    virtual bool enqueueJob(const QString &type, QByteArray payloadCiphertext, qint64 nextAttemptMs) = 0;
    virtual QVector<JobRecord> dueJobs(qint64 nowMs) const = 0;
    virtual bool markAttempt(qint64 jobId, bool success, qint64 nextAttemptMs) = 0;
};

class ISearchRepository {
public:
    virtual ~ISearchRepository() = default;
    virtual bool insertToken(const QString &conversationId,
                             const QString &messageId,
                             QByteArray tokenDigest,
                             int positionHint) = 0;
    virtual QVector<QString> findMessagesByToken(const QByteArray &tokenDigest) const = 0;
};

class ITrustRepository {
public:
    virtual ~ITrustRepository() = default;
    virtual bool upsertTrust(const domain::DeviceFingerprint &trust) = 0;
    virtual std::optional<domain::DeviceFingerprint> findTrust(const QString &deviceId) const = 0;
};

class SqlAccountsRepository final : public IAccountsRepository {
public:
    explicit SqlAccountsRepository(QSqlDatabase db);

    bool upsertAccount(const domain::Account &account) override;
    std::optional<domain::Account> activeAccount() const override;
    bool deleteAccount(const QString &accountId) override;

private:
    QSqlDatabase m_db;
};

class SqlConversationsRepository final : public IConversationsRepository {
public:
    explicit SqlConversationsRepository(QSqlDatabase db);

    bool upsertConversation(const domain::Conversation &conversation) override;
    QVector<domain::Conversation> listConversations() const override;

private:
    QSqlDatabase m_db;
};

class SqlMessagesRepository final : public IMessagesRepository {
public:
    explicit SqlMessagesRepository(QSqlDatabase db);

    bool upsertMessage(const domain::Message &message) override;
    QVector<domain::Message> listConversationMessages(const QString &conversationId,
                                                      int limit) const override;

private:
    QSqlDatabase m_db;
};

class SqlDevicesRepository final : public IDevicesRepository {
public:
    explicit SqlDevicesRepository(QSqlDatabase db);

    bool upsertDevice(const domain::Device &device) override;
    QVector<domain::Device> listDevicesByUser(const QString &userId) const override;

private:
    QSqlDatabase m_db;
};

class SqlSyncRepository final : public ISyncRepository {
public:
    explicit SqlSyncRepository(QSqlDatabase db);

    bool upsertRecord(const domain::SyncRecord &record) override;
    QVector<domain::SyncRecord> listRecords(const QString &collection) const override;

private:
    QSqlDatabase m_db;
};

class SqlJobsRepository final : public IJobsRepository {
public:
    explicit SqlJobsRepository(QSqlDatabase db);

    bool enqueueJob(const QString &type, QByteArray payloadCiphertext, qint64 nextAttemptMs) override;
    QVector<JobRecord> dueJobs(qint64 nowMs) const override;
    bool markAttempt(qint64 jobId, bool success, qint64 nextAttemptMs) override;

private:
    QSqlDatabase m_db;
};

class SqlSearchRepository final : public ISearchRepository {
public:
    explicit SqlSearchRepository(QSqlDatabase db);

    bool insertToken(const QString &conversationId,
                     const QString &messageId,
                     QByteArray tokenDigest,
                     int positionHint) override;
    QVector<QString> findMessagesByToken(const QByteArray &tokenDigest) const override;

private:
    QSqlDatabase m_db;
};

class SqlTrustRepository final : public ITrustRepository {
public:
    explicit SqlTrustRepository(QSqlDatabase db);

    bool upsertTrust(const domain::DeviceFingerprint &trust) override;
    std::optional<domain::DeviceFingerprint> findTrust(const QString &deviceId) const override;

private:
    QSqlDatabase m_db;
};

} // namespace vox::storage

#endif // VOX_STORAGE_REPOSITORIES_HPP
