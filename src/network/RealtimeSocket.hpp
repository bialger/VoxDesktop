#ifndef VOX_NETWORK_REALTIMESOCKET_HPP
#define VOX_NETWORK_REALTIMESOCKET_HPP

#include <QObject>
#include <QTimer>
#include <QUrl>
#ifdef VOX_HAS_QT_WEBSOCKETS
#include <QWebSocket>
#endif

#include "network/ApiDtos.hpp"

namespace vox::network {

class RealtimeSocket final : public QObject {
    Q_OBJECT

public:
    explicit RealtimeSocket(QObject *parent = nullptr);

    void connectAndAuthenticate(const QUrl &url, const QString &accessToken);
    void close();

signals:
    void envelopeReceived(const EnvelopeDto &envelope);
    void membershipChanged(const QString &conversationId, int membershipVersion);
    void userDevicesChanged(const QString &userId);
    void syncRecordChanged(const QString &collection, const QString &recordId, int version);
    void socketError(const QString &message);

private:
    void onConnected();
    void onTextMessage(const QString &message);
    void onDisconnected();
    void scheduleReconnect();

#ifdef VOX_HAS_QT_WEBSOCKETS
    QWebSocket m_socket;
#endif
    QTimer m_reconnectTimer;
    QUrl m_url;
    QString m_accessToken;
    int m_reconnectAttempt{0};
};

} // namespace vox::network

#endif // VOX_NETWORK_REALTIMESOCKET_HPP
