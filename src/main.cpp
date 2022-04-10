#include <iop/loop.hpp>

#include <dallas_temperature.hpp>
#include <dht.hpp>
#include <soil_resistivity.hpp>
#include <factory_reset_button.hpp>

#include "configuration.hpp"

constexpr const uint32_t interval = 180 * 1000;

sensor::SoilResistivity soilResistivity(config::soilResistivityPower);
dallas::TemperatureCollection soilTemperature(config::soilTemperature);
dht::Dht airTempAndHumidity(config::airTempAndHumidity, config::dhtVersion);

namespace iop {
auto reportMeasurements(iop::EventLoop &loop, const iop::AuthToken &token) noexcept -> void;

auto setup(iop::EventLoop &loop) noexcept -> void {
  reset::setup(config::factoryResetButton);
  soilResistivity.begin();
  soilTemperature.begin();
  airTempAndHumidity.begin();

  loop.setAuthenticatedInterval(interval, reportMeasurements);
  loop.setInterval(1000, reset::resetIfNeeded);
}

auto reportMeasurements(iop::EventLoop &loop, const iop::AuthToken &token) noexcept -> void {
  loop.logger().debug(IOP_STR("Handle Measurements"));

  const auto make = [](JsonDocument &doc) {
    doc["air_temperature_celsius"] = airTempAndHumidity.measureTemperature();
    doc["air_humidity_percentage"] = airTempAndHumidity.measureHumidity();
    doc["air_heat_index_celsius"] = airTempAndHumidity.measureHeatIndex();
    doc["soil_temperature_celsius"] = soilTemperature.measure();
    doc["soil_resistivity_raw"] = soilResistivity.measure();
  };
  const auto json = loop.api().makeJson(IOP_STR("EventLoop::handleMeasurements"), make);
  if (!json) {
    loop.logger().error(IOP_STR("Unable to send measurements"));
    iop_panic(IOP_STR("reportsMeasurement authenticated task event buffer overflow"));
  }

  loop.registerEvent(token, *json);
}
}