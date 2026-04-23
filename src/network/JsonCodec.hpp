#ifndef VOX_NETWORK_JSONCODEC_HPP
#define VOX_NETWORK_JSONCODEC_HPP

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <optional>

namespace vox::network {

class JsonCodec final {
public:
    static std::optional<QJsonObject> parseObject(QByteArrayView bytes, QString *error = nullptr);
    static QByteArray toBytes(const QJsonObject &object);

    static std::optional<QString> requiredString(const QJsonObject &obj,
                                                 const QString &key,
                                                 QString *error = nullptr);

    static std::optional<qint64> requiredInteger(const QJsonObject &obj,
                                                 const QString &key,
                                                 QString *error = nullptr);
};

} // namespace vox::network

#endif // VOX_NETWORK_JSONCODEC_HPP
