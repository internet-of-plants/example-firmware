#include <iop/loop.hpp>
#include <pin.hpp>
//#include <dallas_temperature.hpp>
//#include <dht.hpp>
//#include <factory_reset_button.hpp>
//#include <soil_resistivity.hpp>

namespace config {
constexpr static iop::time::milliseconds measurementsInterval = 180 * 1000;
constexpr static char SSID_RAW[] IOP_ROM = "SSID";
static const iop::StaticString SSID = reinterpret_cast<const __FlashStringHelper*>(SSID_RAW);
constexpr static char PSK_RAW[] IOP_ROM = "CKyiYiPBpigVi6AxjVg2";
static const iop::StaticString PSK = reinterpret_cast<const __FlashStringHelper*>(PSK_RAW);
//constexpr static Pin airTempAndHumidity = Pin::D6;
//constexpr static dht::Version dhtVersion = dht::Version::DHT22;
//constexpr static Pin factoryResetButton = Pin::D1;
//constexpr static Pin soilResistivityPower = Pin::D7;
//constexpr static Pin soilTemperature = Pin::D5;
}

//static dallas::TemperatureCollection soilTemperature(IOP_PIN_RAW(config::soilTemperature));
//static dht::Dht airTempAndHumidity(IOP_PIN_RAW(config::airTempAndHumidity), config::dhtVersion);
//static sensor::SoilResistivity soilResistivity(IOP_PIN_RAW(config::soilResistivityPower));

auto prepareJson(iop::EventLoop & loop) noexcept -> std::unique_ptr<iop::Api::Json> {
  loop.logger().infoln(IOP_STR("Handle Measurements"));
  auto json = loop.api().makeJson(IOP_FUNC, [](JsonDocument &doc) {
    //doc["air_temperature_celsius"] = airTempAndHumidity.measureTemperature();
    //doc["air_humidity_percentage"] = airTempAndHumidity.measureHumidity();
    //doc["soil_resistivity_raw"] = soilResistivity.measure();
    //doc["soil_temperature_celsius"] = soilTemperature.measure();
  });
  if (!json) iop_panic(IOP_STR("Unable to send measurements, buffer overflow"));
  return json;
}

auto reportMeasurements(iop::EventLoop &loop, const iop::AuthToken &token) noexcept -> void {
  IOP_TRACE();
  loop.registerEvent(token, *prepareJson(loop));
}

namespace iop {
auto setup(EventLoop &loop) noexcept -> void {
  loop.setAccessPointCredentials(config::SSID, config::PSK);
  //airTempAndHumidity.begin();
  //loop.setInterval(1000, reset::resetIfNeeded);
  //reset::setup(IOP_PIN_RAW(config::factoryResetButton));
  //soilResistivity.begin();
  //soilTemperature.begin();
  loop.setAuthenticatedInterval(config::measurementsInterval, reportMeasurements);
}
}
