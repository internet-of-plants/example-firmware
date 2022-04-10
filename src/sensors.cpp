#include "sensors.hpp"

void Sensors::setup() noexcept {
  this->soilResistivity.begin();
  this->airTempAndHumidity.begin();
  this->soilTemperature.begin();
}

auto Sensors::measure() noexcept -> Event {
  return Event(
    this->airTempAndHumidity.measureTemperature(),
    this->airTempAndHumidity.measureHumidity(),
    this->airTempAndHumidity.measureHeatIndex(),
    this->soilTemperature.measure(),
    this->soilResistivity.measure()
  );
}

Sensors::Sensors(const iop_hal::io::Pin soilResistivityPower, const iop_hal::io::Pin soilTemperature, const iop_hal::io::Pin dht, const uint8_t dhtVersion) noexcept
      : soilResistivity(soilResistivityPower),
        soilTemperature(soilTemperature),
        airTempAndHumidity(dht, dhtVersion) {}