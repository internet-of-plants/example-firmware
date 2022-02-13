#include "driver/upgrade.hpp"

namespace driver {
auto upgrade(const iop::Network &network, const iop::StaticString path, const std::string_view authorization_header) noexcept -> iop::NetworkStatus {
    (void) network;
    (void) path;
    (void) authorization_header;
    return iop::NetworkStatus::OK;
}
} // namespace driver