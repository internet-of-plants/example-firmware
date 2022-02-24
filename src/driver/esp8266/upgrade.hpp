#include "driver/upgrade.hpp"
#include "driver/client.hpp"
#include "driver/panic.hpp"
#include "driver/network.hpp"

#include <memory>

#include "ESP8266httpUpdate.h"


namespace driver {
auto Upgrade::run(const iop::Network &network, const iop::StaticString path, const std::string_view authorization_header) noexcept -> driver::UpgradeStatus {
  auto *client = iop::wifi.client;
  iop_assert(client, IOP_STR("Wifi has been moved out, client is nullptr"));

  auto ESPhttpUpdate = std::unique_ptr<ESP8266HTTPUpdate>(new (std::nothrow) ESP8266HTTPUpdate());
  iop_assert(ESPhttpUpdate, IOP_STR("Unable to allocate ESP8266HTTPUpdate"));
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
    return driver::UpgradeStatus::NO_UPGRADE;

  case HTTP_UPDATE_FAILED:
    // TODO(pc): properly handle ESPhttpUpdate.getLastError()
    network.logger().error(IOP_STR("Update failed: "),
                       std::string_view(ESPhttpUpdate->getLastErrorString().c_str()));
    return driver::UpgradeStatus::BROKEN_SERVER;
}

// TODO(pc): properly handle ESPhttpUpdate.getLastError()
network.logger().error(IOP_STR("Update failed (UNKNOWN): "),
                    std::string_view(ESPhttpUpdate->getLastErrorString().c_str()));
return driver::UpgradeStatus::BROKEN_SERVER;
}
} // namespace driver