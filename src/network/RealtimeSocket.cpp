#include "network/RealtimeSocket.hpp"

#include "network/JsonCodec.hpp"

#include <algorithm>
#include <QJsonDocument>
#include <QJsonObject>
#ifdef VOX_HAS_QT_WEBSOCKETS
#include <QNetworkRequest>
#endif

namespace vox::network {

RealtimeSocket::RealtimeSocket(QObject *parent) : QObject(parent) {
    m_reconnectTimer.setSingleShot(true);

#ifdef VOX_HAS_QT_WEBSOCKETS
    connect(&m_socket, &QWebSocket::connected, this, &RealtimeSocket::onConnected);
    connect(&m_socket, &QWebSocket::textMessageReceived, this, &RealtimeSocket::onTextMessage);
    connect(&m_socket, &QWebSocket::disconnected, this, &RealtimeSocket::onDisconnected);
    connect(&m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this,
            [this](QAbstractSocket::SocketError) { emit socketError(m_socket.errorString()); });
#endif

    connect(&m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (m_url.isValid()) {
            connectAndAuthenticate(m_url, m_accessToken);
        }
    });
}

void RealtimeSocket::connectAndAuthenticate(const QUrl &url, const QString &accessToken) {
    m_url = url;
    m_accessToken = accessToken;

#ifdef VOX_HAS_QT_WEBSOCKETS
    QNetworkRequest request(url);
    if (!accessToken.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());
    }

    m_socket.open(request);
#else
    emit socketError("Qt WebSockets module is not available in this build.");
#endif
}

void RealtimeSocket::close() {
    m_reconnectTimer.stop();
#ifdef VOX_HAS_QT_WEBSOCKETS
    m_socket.close();
#endif
}

void RealtimeSocket::onConnected() {
    m_reconnectAttempt = 0;
}

void RealtimeSocket::onTextMessage(const QString &message) {
    const auto object = JsonCodec::parseObject(message.toUtf8());
    if (!object.has_value()) {
        return;
    }

    const QString type = object->value("type").toString();

    if (type == "envelope") {
        const auto envelope = parseEnvelopeDto(*object);
        if (envelope.has_value()) {
            emit envelopeReceived(*envelope);
        }
        return;
    }

    if (type == "conversation_membership_changed") {
        emit membershipChanged(object->value("conversation_id").toString(),
                               object->value("membership_version").toInt());
        return;
    }

    if (type == "user_devices_changed") {
        emit userDevicesChanged(object->value("user_id").toString());
        return;
    }

    if (type == "sync_record_changed") {
        emit syncRecordChanged(object->value("collection").toString(),
                               object->value("record_id").toString(),
                               object->value("version").toInt());
    }
}

void RealtimeSocket::onDisconnected() {
    scheduleReconnect();
}

void RealtimeSocket::scheduleReconnect() {
#ifndef VOX_HAS_QT_WEBSOCKETS
    return;
#else
    ++m_reconnectAttempt;
    const int capped = std::min(m_reconnectAttempt, 6);
    const int delayMs = (1 << capped) * 250;
    m_reconnectTimer.start(delayMs);
#endif
}

} // namespace vox::network
