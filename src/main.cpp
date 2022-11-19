#include <iop/loop.hpp>
#include <dallas_temperature.hpp>
#include <dht.hpp>
#include <factory_reset_button.hpp>
#include <soil_resistivity.hpp>
#include <light.hpp>

#include "pin.hpp"
#include "generated/psk.hpp"

namespace config {
constexpr static iop::time::milliseconds measurementsInterval = 180 * 1000;
constexpr static iop::time::milliseconds unauthenticatedActionsInterval = 1000;
constexpr static int8_t timezone = -3;

static char SSID_RAW[] IOP_ROM = "iop";
static const iop::StaticString SSID = reinterpret_cast<const __FlashStringHelper*>(SSID_RAW);
static const iop::StaticString PSK = reinterpret_cast<const __FlashStringHelper*>(generated::PSK);

static const Pin light = Pin::D4;
static const std::pair<relay::Moment, relay::State> lightMoments[] = {
  std::make_pair(relay::Moment(6, 0, 0), relay::State::ON),
  std::make_pair(relay::Moment(0, 0, 0), relay::State::OFF),
};
static const Pin airTempAndHumidity = Pin::D6;
static const dht::Version dhtVersion = dht::Version::DHT22;
static const Pin factoryResetButton = Pin::D1;
static const Pin soilResistivityPower = Pin::D7;
static const Pin soilTemperature = Pin::D5;
}

static relay::Light light(IOP_PIN_RAW(config::light));
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
  iop_assert(json, IOP_STR("Unable to generate request payload, OOM or buffer overflow"));
  return json;
}

auto reportMeasurements(iop::EventLoop &loop, const iop::AuthToken &token) noexcept -> void {
  loop.registerEvent(token, *prepareJson(loop));
}

auto cleanup() noexcept -> void {
  light.reset();
}

auto unauthenticatedAct(iop::EventLoop &loop) noexcept -> void {
  reset::resetIfNeeded(loop);
  light.actIfNeeded();
}

namespace iop {
auto setup(EventLoop &loop) noexcept -> void {
  loop.setAccessPointCredentials(config::SSID, config::PSK);
  for (const auto [moment, state]: config::lightMoments) {
    light.setTime(moment, state);
  }
  airTempAndHumidity.begin();
  reset::setup(IOP_PIN_RAW(config::factoryResetButton));
  soilResistivity.begin();
  soilTemperature.begin();

  loop.setCleanup(cleanup);
  loop.setTimezone(config::timezone);
  loop.setInterval(config::unauthenticatedActionsInterval, unauthenticatedAct);
  loop.setAuthenticatedInterval(config::measurementsInterval, reportMeasurements);
}
}
