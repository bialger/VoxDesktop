#include "network/SyncApi.hpp"

#include "network/ApiPaths.hpp"
#include "network/JsonCodec.hpp"

#include <QJsonArray>
#include <QUrlQuery>

namespace vox::network {

SyncApi::SyncApi(NetworkAccess &network) : m_network(network) {}

ApiResult<SyncKeyBundleDto> SyncApi::getKeyBundle() const {
    const auto response = m_network.get(ApiPaths::kSyncKeyBundle);

    ApiResult<SyncKeyBundleDto> result;
    result.statusCode = response.statusCode;

    const auto object = JsonCodec::parseObject(response.body, &result.error);
    if (!object.has_value()) {
        return result;
    }

    if (response.ok()) {
        result.data = parseSyncKeyBundleDto(*object);
        result.ok = result.data.has_value();
        if (!result.ok) {
            result.error = "Invalid sync key bundle payload";
        }
        return result;
    }

    const auto apiError = parseApiError(*object);
    result.error = apiError.has_value() ? apiError->message : response.errorMessage;
    return result;
}

VoidResult SyncApi::putKeyBundle(const SyncKeyBundleDto &bundle) const {
    QJsonObject body;
    body.insert("version", bundle.version);
    body.insert("wrapped_sync_key", bundle.wrappedSyncKey);
    body.insert("sync_wrap_salt", bundle.syncWrapSalt);
    body.insert("sync_wrap_params", bundle.syncWrapParams);

    const auto response = m_network.putJson(ApiPaths::kSyncKeyBundle, JsonCodec::toBytes(body));

    VoidResult result;
    result.statusCode = response.statusCode;
    result.ok = response.ok();
    if (!result.ok) {
        const auto object = JsonCodec::parseObject(response.body, &result.error);
        if (object.has_value()) {
            const auto apiError = parseApiError(*object);
            if (apiError.has_value()) {
                result.error = apiError->message;
            }
        }
    }

    return result;
}

ApiResult<SyncChangesResponse> SyncApi::changes(const QString &collection,
                                                const QString &cursor,
                                                int limit) const {
    QUrlQuery query;
    query.addQueryItem("collection", collection);
    query.addQueryItem("limit", QString::number(limit));
    if (!cursor.isEmpty()) {
        query.addQueryItem("cursor", cursor);
    }

    const auto response = m_network.get(ApiPaths::kSyncChanges, query);

    ApiResult<SyncChangesResponse> result;
    result.statusCode = response.statusCode;

    const auto object = JsonCodec::parseObject(response.body, &result.error);
    if (!object.has_value()) {
        return result;
    }

    if (response.ok()) {
        SyncChangesResponse parsed;
        const auto recordsArray = object->value("records").toArray();
        parsed.records.reserve(recordsArray.size());
        for (const auto &entry : recordsArray) {
            const auto record = parseSyncRecordDto(entry.toObject());
            if (!record.has_value()) {
                result.error = "Invalid sync record entry";
                return result;
            }
            parsed.records.push_back(*record);
        }

        parsed.nextCursor = object->value("next_cursor").toString();
        parsed.hasMore = object->value("has_more").toBool(false);

        result.ok = true;
        result.data = parsed;
        return result;
    }

    const auto apiError = parseApiError(*object);
    result.error = apiError.has_value() ? apiError->message : response.errorMessage;
    return result;
}

VoidResult SyncApi::putRecord(const SyncRecordDto &record) const {
    const QString path = ApiPaths::kSyncRecordsPrefix + record.collection + "/" + record.recordId;
    const auto response = m_network.putJson(path, JsonCodec::toBytes(toJson(record)));

    VoidResult result;
    result.statusCode = response.statusCode;
    result.ok = response.ok();

    if (!result.ok) {
        const auto object = JsonCodec::parseObject(response.body, &result.error);
        if (object.has_value()) {
            const auto apiError = parseApiError(*object);
            if (apiError.has_value()) {
                result.error = apiError->message;
            }
        }
    }

    return result;
}

VoidResult SyncApi::deleteRecord(const QString &collection, const QString &recordId) const {
    const auto response = m_network.deleteRequest(ApiPaths::kSyncRecordsPrefix + collection + "/" + recordId);

    VoidResult result;
    result.statusCode = response.statusCode;
    result.ok = response.ok();

    if (!result.ok) {
        const auto object = JsonCodec::parseObject(response.body, &result.error);
        if (object.has_value()) {
            const auto apiError = parseApiError(*object);
            if (apiError.has_value()) {
                result.error = apiError->message;
            }
        }
    }

    return result;
}

} // namespace vox::network
