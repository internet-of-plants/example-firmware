
#include "driver/network.hpp"

namespace iop {
void Network::setup() const noexcept { IOP_TRACE(); }
auto Network::httpRequest(const HttpMethod method_,
                          const std::optional<std::string_view> &token, StaticString path,
                          const std::optional<std::string_view> &data) const noexcept
    -> Response {
  (void)this;
  (void)token;
  (void)method_;
  (void)path;
  (void)data;
  IOP_TRACE();
  return Response(NetworkStatus::OK, {});
}
}