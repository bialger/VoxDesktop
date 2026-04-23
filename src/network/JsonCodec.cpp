#include "network/JsonCodec.hpp"

#include <QJsonDocument>
#include <QJsonParseError>

namespace vox::network {

std::optional<QJsonObject> JsonCodec::parseObject(QByteArrayView bytes, QString *error) {
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(bytes.toByteArray(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error != nullptr) {
            *error = parseError.errorString();
        }
        return std::nullopt;
    }
    return doc.object();
}

QByteArray JsonCodec::toBytes(const QJsonObject &object) {
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

std::optional<QString> JsonCodec::requiredString(const QJsonObject &obj,
                                                 const QString &key,
                                                 QString *error) {
    const auto value = obj.value(key);
    if (!value.isString()) {
        if (error != nullptr) {
            *error = QString("Missing string field '%1'").arg(key);
        }
        return std::nullopt;
    }
    return value.toString();
}

std::optional<qint64> JsonCodec::requiredInteger(const QJsonObject &obj,
                                                 const QString &key,
                                                 QString *error) {
    const auto value = obj.value(key);
    if (!value.isDouble()) {
        if (error != nullptr) {
            *error = QString("Missing integer field '%1'").arg(key);
        }
        return std::nullopt;
    }
    return value.toInteger();
}

} // namespace vox::network
