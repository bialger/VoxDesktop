#ifndef VOX_PLATFORM_SECUREWRAP_HPP
#define VOX_PLATFORM_SECUREWRAP_HPP

#include <QByteArray>
#include <QString>
#include <optional>

namespace vox::platform {

class SecureWrap {
public:
  virtual ~SecureWrap() = default;

  virtual bool store(const QString &key, QByteArray value) = 0;
  virtual std::optional<QByteArray> load(const QString &key) const = 0;
};

} // namespace vox::platform

#endif // VOX_PLATFORM_SECUREWRAP_HPP
