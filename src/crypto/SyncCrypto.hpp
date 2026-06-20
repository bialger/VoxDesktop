#ifndef VOX_CRYPTO_SYNCCRYPTO_HPP
#define VOX_CRYPTO_SYNCCRYPTO_HPP

#include <QByteArray>
#include <QJsonObject>
#include <optional>

namespace vox::crypto {

struct SyncWrapParams {
  int memoryKiB{65536};
  int iterations{3};
  int parallelism{1};

  QJsonObject toJson() const;
  static SyncWrapParams fromJson(const QJsonObject &obj);
};

struct WrappedSyncKeyBundle {
  QByteArray wrappedSyncKey;
  QByteArray salt;
  SyncWrapParams params;
};

class SyncCrypto final {
public:
  static std::optional<WrappedSyncKeyBundle> wrapSyncMasterKey(QByteArrayView passwordDerived,
                                                               QByteArrayView syncMasterKey,
                                                               SyncWrapParams params = {});

  static std::optional<QByteArray> unwrapSyncMasterKey(QByteArrayView passwordDerived,
                                                       const WrappedSyncKeyBundle &bundle);
};

} // namespace vox::crypto

#endif // VOX_CRYPTO_SYNCCRYPTO_HPP
