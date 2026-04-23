#include "crypto/SyncCrypto.hpp"

#include "crypto/CryptoHelpers.hpp"

#include <sodium.h>

namespace vox::crypto {

QJsonObject SyncWrapParams::toJson() const {
    QJsonObject json;
    json.insert("algorithm", "argon2id");
    json.insert("memory_kib", memoryKiB);
    json.insert("iterations", iterations);
    json.insert("parallelism", parallelism);
    return json;
}

SyncWrapParams SyncWrapParams::fromJson(const QJsonObject &obj) {
    SyncWrapParams params;
    params.memoryKiB = obj.value("memory_kib").toInt(params.memoryKiB);
    params.iterations = obj.value("iterations").toInt(params.iterations);
    params.parallelism = obj.value("parallelism").toInt(params.parallelism);
    return params;
}

std::optional<WrappedSyncKeyBundle> SyncCrypto::wrapSyncMasterKey(QByteArrayView passwordDerived,
                                                                  QByteArrayView syncMasterKey,
                                                                  SyncWrapParams params) {
    if (passwordDerived.empty() || syncMasterKey.size() != 32) {
        return std::nullopt;
    }

    const QByteArray salt = CryptoHelpers::randomBytes(crypto_pwhash_SALTBYTES);
    if (salt.size() != crypto_pwhash_SALTBYTES) {
        return std::nullopt;
    }

    const auto kek = CryptoHelpers::argon2idKdf(passwordDerived,
                                                salt,
                                                static_cast<quint64>(params.iterations),
                                                static_cast<size_t>(params.memoryKiB) * 1024,
                                                32);
    if (kek.size() != 32) {
        return std::nullopt;
    }

    const QByteArray nonce = CryptoHelpers::randomBytes(24);
    if (nonce.size() != 24) {
        return std::nullopt;
    }

    const auto wrapped = CryptoHelpers::xchachaEncryptLocal(kek, nonce, syncMasterKey, "vox-sync-v1");
    if (wrapped.isEmpty()) {
        return std::nullopt;
    }

    QByteArray serialized = nonce;
    serialized.append(wrapped);
    return WrappedSyncKeyBundle{serialized, salt, params};
}

std::optional<QByteArray> SyncCrypto::unwrapSyncMasterKey(QByteArrayView passwordDerived,
                                                          const WrappedSyncKeyBundle &bundle) {
    if (passwordDerived.empty() || bundle.wrappedSyncKey.size() < 24 + 16) {
        return std::nullopt;
    }

    const auto kek = CryptoHelpers::argon2idKdf(passwordDerived,
                                                bundle.salt,
                                                static_cast<quint64>(bundle.params.iterations),
                                                static_cast<size_t>(bundle.params.memoryKiB) * 1024,
                                                32);
    if (kek.size() != 32) {
        return std::nullopt;
    }

    const QByteArray nonce = bundle.wrappedSyncKey.left(24);
    const QByteArray ciphertext = bundle.wrappedSyncKey.mid(24);

    return CryptoHelpers::xchachaDecryptLocal(kek, nonce, ciphertext, "vox-sync-v1");
}

} // namespace vox::crypto
