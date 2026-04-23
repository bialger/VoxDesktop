#ifndef VOX_NETWORK_NETWORKACCESS_HPP
#define VOX_NETWORK_NETWORKACCESS_HPP

#include <QByteArray>
#include <QHash>
#include <QNetworkRequest>
#include <QString>
#include <QUrlQuery>
#include <functional>

namespace vox::network {

struct NetworkResponse {
    int statusCode{0};
    QByteArray body;
    QString errorMessage;

    [[nodiscard]] bool ok() const;
};

class NetworkAccess final {
public:
    using HeaderMap = QHash<QString, QString>;
    using Handler =
        std::function<NetworkResponse(const QString &, const QString &, const QByteArray &, const HeaderMap &)>;

    explicit NetworkAccess(QString baseUrl);

    void setBearerToken(QString token);
    void setCustomHandler(Handler handler);

    NetworkResponse get(const QString &path,
                        const QUrlQuery &query = {},
                        const HeaderMap &headers = {}) const;

    NetworkResponse postJson(const QString &path,
                             const QByteArray &jsonBody,
                             const QUrlQuery &query = {},
                             const HeaderMap &headers = {}) const;

    NetworkResponse putJson(const QString &path,
                            const QByteArray &jsonBody,
                            const QUrlQuery &query = {},
                            const HeaderMap &headers = {}) const;

    NetworkResponse putBytes(const QString &path,
                             const QByteArray &payload,
                             const QUrlQuery &query = {},
                             const HeaderMap &headers = {}) const;

    NetworkResponse postBytes(const QString &path,
                              const QByteArray &payload,
                              const QUrlQuery &query = {},
                              const HeaderMap &headers = {}) const;

    NetworkResponse deleteRequest(const QString &path,
                                  const QUrlQuery &query = {},
                                  const HeaderMap &headers = {}) const;

private:
    NetworkResponse execute(const QString &method,
                            const QString &path,
                            const QByteArray &body,
                            const QUrlQuery &query,
                            const HeaderMap &headers,
                            const QByteArray &contentType) const;

    QString m_baseUrl;
    QString m_bearerToken;
    Handler m_customHandler;
};

} // namespace vox::network

#endif // VOX_NETWORK_NETWORKACCESS_HPP
