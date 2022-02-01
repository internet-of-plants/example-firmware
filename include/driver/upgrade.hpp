#include <string_view>
#include "driver/network.hpp"

namespace driver {
auto upgrade(const iop::Network &network, iop::StaticString path, std::string_view authorization_header) noexcept -> iop::NetworkStatus;
}