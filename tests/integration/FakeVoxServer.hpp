#ifndef VOX_TESTS_INTEGRATION_FAKEVOXSERVER_HPP
#define VOX_TESTS_INTEGRATION_FAKEVOXSERVER_HPP

#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QByteArrayView>
#include <optional>

#include "network/NetworkAccess.hpp"

namespace network = vox::network;

class FakeVoxServer {
public:
    struct Envelope {
        QString envelopeId;
        QString conversationId;
        QString senderUserId;
        QString senderDeviceId;
        QString ciphertext;
        qint64 serverTimestamp{0};
        int envelopeType{0};
    };

    network::NetworkResponse handle(const QString &method,
                                    const QString &path,
                                    const QByteArray &body,
                                    const network::NetworkAccess::HeaderMap &headers);

    void setForceUnauthorized(bool value);
    void setFailNextSend(bool value);

private:
    network::NetworkResponse ok(const QJsonObject &obj, int status = 200) const;
    network::NetworkResponse error(int status, int code, const QString &message) const;

    std::optional<QJsonObject> parseJson(QByteArrayView bytes) const;
    QString authUserFromHeaders(const network::NetworkAccess::HeaderMap &headers) const;

    struct AccountState {
        QString userId;
        QString username;
        QString passwordDerived;
        QString deviceId;
        QString accessToken;
        QString refreshToken;
    };

    bool m_forceUnauthorized{false};
    bool m_failNextSend{false};
    int m_userSeq{1};
    int m_envelopeSeq{1};
    int m_attachmentSeq{1};

    QHash<QString, AccountState> m_usersByUsername;
    QHash<QString, QString> m_userByAccessToken;
    QHash<QString, QString> m_userByRefreshToken;
    QHash<QString, QByteArray> m_attachments;
    QVector<Envelope> m_envelopes;
};

#endif // VOX_TESTS_INTEGRATION_FAKEVOXSERVER_HPP
