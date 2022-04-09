#include "iop/log.hpp"
#include "sensors.hpp"
#include "utils.hpp"

#ifdef IOP_SENSORS
#include "iop/panic.hpp"

void Sensors::setup() noexcept {
  IOP_TRACE();
  this->soilResistivity.begin();
  this->airTempAndHumidity.begin();
  this->soilTemperature.begin();
}

auto Sensors::measure() noexcept -> Event {
  IOP_TRACE();
  return (Event) {
    .airTemperatureCelsius = this->airTempAndHumidity.measureTemperature(),
    .airHumidityPercentage = this->airTempAndHumidity.measureHumidity(),
    .airHeatIndexCelsius = this->airTempAndHumidity.measureHeatIndex(),
    .soilTemperatureCelsius = this->soilTemperature.measure(),
    .soilResistivityRaw = this->soilResistivity.measure(),
  };
}
#else
void Sensors::setup() noexcept {
  IOP_TRACE();
  (void)*this;
}
auto Sensors::measure() noexcept -> Event {
  IOP_TRACE();
  (void)*this;
  return Event(0.0, 0.0, 0.0, 0.0, 0);
}
#endif

Sensors::Sensors(const driver::io::Pin soilResistivityPower, const driver::io::Pin soilTemperature, const driver::io::Pin dht, const uint8_t dhtVersion) noexcept
      : soilResistivity(soilResistivityPower),
        soilTemperature(soilTemperature),
        airTempAndHumidity(dht, dhtVersion) {}