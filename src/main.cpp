#include <iop/loop.hpp>
#include <dallas_temperature.hpp>
#include <dht.hpp>
#include <factory_reset_button.hpp>
#include <soil_resistivity.hpp>

#include "pin.hpp"
#include "generated/psk.hpp"

namespace config {
constexpr static iop::time::milliseconds measurementsInterval = 180 * 1000;
constexpr static char SSID_RAW[] IOP_ROM = "iop";
static const iop::StaticString SSID = reinterpret_cast<const __FlashStringHelper*>(SSID_RAW);
static const iop::StaticString PSK = reinterpret_cast<const __FlashStringHelper*>(generated::PSK);

constexpr static Pin airTempAndHumidity = Pin::D6;
constexpr static dht::Version dhtVersion = dht::Version::DHT22;
constexpr static Pin factoryResetButton = Pin::D1;
constexpr static Pin soilResistivityPower = Pin::D7;
constexpr static Pin soilTemperature = Pin::D5;
}

static dallas::TemperatureCollection soilTemperature(IOP_PIN_RAW(config::soilTemperature));
static dht::Dht airTempAndHumidity(IOP_PIN_RAW(config::airTempAndHumidity), config::dhtVersion);
static sensor::SoilResistivity soilResistivity(IOP_PIN_RAW(config::soilResistivityPower));

auto prepareJson(iop::EventLoop & loop) noexcept -> std::unique_ptr<iop::Api::Json> {
  IOP_TRACE();
  loop.logger().infoln(IOP_STR("Handle Measurements"));
  auto json = loop.api().makeJson(IOP_FUNC, [](JsonDocument &doc) {
    doc["air_temperature_celsius"] = airTempAndHumidity.measureTemperature();
    doc["air_humidity_percentage"] = airTempAndHumidity.measureHumidity();
    doc["soil_resistivity_raw"] = soilResistivity.measure();
    doc["soil_temperature_celsius"] = soilTemperature.measure();
  });
  iop_assert(json, IOP_STR("Unable to send measurements, OOM or buffer overflow"));
  return json;
}

auto reportMeasurements(iop::EventLoop &loop, const iop::AuthToken &token) noexcept -> void {
  loop.registerEvent(token, *prepareJson(loop));
}

namespace iop {
auto setup(EventLoop &loop) noexcept -> void {
  loop.setAccessPointCredentials(config::SSID, config::PSK);
  airTempAndHumidity.begin();
  loop.setInterval(1000, reset::resetIfNeeded);
  reset::setup(IOP_PIN_RAW(config::factoryResetButton));
  soilResistivity.begin();
  soilTemperature.begin();
  loop.setAuthenticatedInterval(config::measurementsInterval, reportMeasurements);
}
}
