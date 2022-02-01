#include "driver/upgrade.hpp"
#include "driver/client.hpp"
#include "driver/panic.hpp"

#include <memory>

#include "ESP8266httpUpdate.h"


namespace driver {
auto upgrade(const iop::Network &network, const iop::StaticString path, const std::string_view authorization_header) noexcept -> iop::NetworkStatus {
  auto *client = iop::data.wifi.client;
  iop_assert(client, IOP_STATIC_STRING("Wifi has been moved out, client is nullptr"));

  auto ESPhttpUpdate = std::unique_ptr<ESP8266HTTPUpdate>(new (std::nothrow) ESP8266HTTPUpdate());
  iop_assert(ESPhttpUpdate, IOP_STATIC_STRING("Unable to allocate ESP8266HTTPUpdate"));
  ESPhttpUpdate->setAuthorization(std::string(authorization_header).c_str());
  ESPhttpUpdate->closeConnectionsOnUpdate(true);
  ESPhttpUpdate->rebootOnUpdate(true);
  ESPhttpUpdate->setLedPin(LED_BUILTIN);

  auto route = String();
  route.concat(network.uri().toString().c_str(), network.uri().length());
route.concat(path.toString().c_str(), path.length());
  const auto result = ESPhttpUpdate->update(*client, route, "");

switch (result) {
  case HTTP_UPDATE_NO_UPDATES:
  case HTTP_UPDATE_OK:
    return iop::NetworkStatus::OK;

  case HTTP_UPDATE_FAILED:
    // TODO(pc): properly handle ESPhttpUpdate.getLastError()
    network.logger().error(IOP_STATIC_STRING("Update failed: "),
                       std::string_view(ESPhttpUpdate->getLastErrorString().c_str()));
    return iop::NetworkStatus::BROKEN_SERVER;
}

// TODO(pc): properly handle ESPhttpUpdate.getLastError()
network.logger().error(IOP_STATIC_STRING("Update failed (UNKNOWN): "),
                    std::string_view(ESPhttpUpdate->getLastErrorString().c_str()));
return iop::NetworkStatus::BROKEN_SERVER;
}
} // namespace driver