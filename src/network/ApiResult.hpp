#ifndef VOX_NETWORK_APIRESULT_HPP
#define VOX_NETWORK_APIRESULT_HPP

#include <QString>
#include <optional>

namespace vox::network {

struct VoidResult {
  bool ok{false};
  int statusCode{0};
  QString error;
};

template<typename T>
struct ApiResult {
  bool ok{false};
  int statusCode{0};
  QString error;
  std::optional<T> data;
};

} // namespace vox::network

#endif // VOX_NETWORK_APIRESULT_HPP
