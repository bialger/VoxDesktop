#ifndef VOX_TESTS_INTEGRATION_FAKEVOXSERVER_HPP
#define VOX_TESTS_INTEGRATION_FAKEVOXSERVER_HPP

#include <QByteArrayView>
#include <QHash>
#include <QJsonObject>
#include <QString>
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

  network::NetworkResponse Handle(const QString &method,
                                  const QString &path,
                                  const QByteArray &body,
                                  const network::NetworkAccess::HeaderMap &headers);

  void SetForceUnauthorized(bool value);
  void SetFailNextSend(bool value);

private:
  [[nodiscard]] network::NetworkResponse Ok(const QJsonObject &obj, int status = 200) const;
  [[nodiscard]] network::NetworkResponse Error(int status, int code, const QString &message) const;

  [[nodiscard]] std::optional<QJsonObject> ParseJson(QByteArrayView bytes) const;
  [[nodiscard]] QString AuthUserFromHeaders(const network::NetworkAccess::HeaderMap &headers) const;

  struct AccountState {
    QString userId;
    QString username;
    QString passwordDerived;
    QString deviceId;
    QString accessToken;
    QString refreshToken;
  };

  bool m_forceUnauthorized_{false};
  bool m_failNextSend_{false};
  int m_userSeq_{1};
  int m_envelopeSeq_{1};
  int m_attachmentSeq_{1};

  QHash<QString, AccountState> m_usersByUsername_;
  QHash<QString, QString> m_userByAccessToken_;
  QHash<QString, QString> m_userByRefreshToken_;
  QHash<QString, QByteArray> m_attachments_;
  QVector<Envelope> m_envelopes_;
};

#endif // VOX_TESTS_INTEGRATION_FAKEVOXSERVER_HPP
