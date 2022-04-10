#include "iop/loop.hpp"
#include "sensors.hpp"
#include "configuration.hpp"

iop::time::milliseconds nextMeasurement;

bool scheduledFactoryReset = false;
Sensors sensors(config::soilResistivityPower, config::soilTemperature, config::airTempAndHumidity, config::dhtVersion);

namespace iop {
auto authenticatedSetup(iop::EventLoop &loop) noexcept -> void {
  (void) loop;
  reset::setup();
  sensors.setup();
}

auto authenticatedLoop(iop::EventLoop &loop, const iop::AuthToken &token) noexcept -> void {
    IOP_TRACE();

    if (scheduledFactoryReset) {
      scheduledFactoryReset = false;
      loop.logger().warn(IOP_STR("Factory Reset: deleting stored credentials"));
      loop.storage().removeWifi();
      loop.storage().removeToken();
      iop::Network::disconnect();
      return;
    }
    
    const auto now = iop_hal::thisThread.timeRunning();
    if (nextMeasurement > now) {
      return;
    }
    
    constexpr const uint32_t interval = 180 * 1000;
    nextMeasurement = now + interval;

    loop.logger().debug(IOP_STR("Handle Measurements"));

    const auto event = sensors.measure();
    const auto make = [&event](JsonDocument &doc) {
      doc["air_temperature_celsius"] = event.airTemperatureCelsius;
      doc["air_humidity_percentage"] = event.airHumidityPercentage;
      doc["air_heat_index_celsius"] = event.airHeatIndexCelsius;
      doc["soil_temperature_celsius"] = event.soilTemperatureCelsius;
      doc["soil_resistivity_raw"] = event.soilResistivityRaw;
    };
    const auto json = loop.api().makeJson(IOP_STR("EventLoop::handleMeasurements"), make);
    if (!json) {
      loop.logger().error(IOP_STR("Unable to send measurements"));
      iop_panic(IOP_STR("Api::registerEvent internal buffer overflow"));
    }

    const auto status = loop.api().registerEvent(token, *json);

    switch (status) {
    case iop::NetworkStatus::FORBIDDEN:
      loop.logger().error(IOP_STR("Unable to send measurements"));
      loop.logger().warn(IOP_STR("Auth token was refused, deleting it"));
      loop.storage().removeToken();
      return;

    case iop::NetworkStatus::BROKEN_CLIENT:
      loop.logger().error(IOP_STR("Unable to send measurements"));
      iop_panic(IOP_STR("Api::registerEvent internal buffer overflow"));

    // Already logged at the Network level
    case iop::NetworkStatus::BROKEN_SERVER:
    case iop::NetworkStatus::IO_ERROR:
      // Nothing to be done besides retrying later

    case iop::NetworkStatus::OK: // Cool beans
      return;
    }

    loop.logger().error(IOP_STR("Unexpected status, EventLoop::handleMeasurements"));
}
}