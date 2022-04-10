#ifndef SENSORS_HPP
#define SENSORS_HPP

#include "iop-hal/io.hpp"

#include <dallas_temperature.hpp>
#include <dht.hpp>
#include <soil_resistivity.hpp>

extern bool scheduledFactoryReset;

/// Monitoring event that is sent to the monitor server
struct Event {
  float airTemperatureCelsius;
  float airHumidityPercentage;
  float airHeatIndexCelsius;
  float soilTemperatureCelsius;
  uint16_t soilResistivityRaw;

  Event(float airTemp, float airHum, float airHeatIndex, float soilTemp, uint16_t soilResistivity) noexcept:
    airTemperatureCelsius(airTemp), airHumidityPercentage(airHum), airHeatIndexCelsius(airHeatIndex), soilTemperatureCelsius(soilTemp), soilResistivityRaw(soilResistivity) {}
};
/// Abstracts away sensors access, providing a cohesive state.
/// It's completely synchronous.
class Sensors {
  sensor::SoilResistivity soilResistivity;
  dallas::TemperatureCollection soilTemperature;
  dht::Dht airTempAndHumidity;

public:
  /// Initializes all sensors
  void setup() noexcept;

  /// Collects a monitoring event from the sensors
  auto measure() noexcept -> Event;

  Sensors(const iop_hal::io::Pin soilResistivityPower,
          const iop_hal::io::Pin soilTemperature, const iop_hal::io::Pin dht,
          const uint8_t dhtVersion) noexcept;

  Sensors(Sensors const &other) noexcept = delete;
  Sensors(Sensors &&other) noexcept = default;
  auto operator=(Sensors const &other) noexcept -> Sensors & = delete;
  auto operator=(Sensors &&other) noexcept -> Sensors & = default;
};

namespace reset {
  /// Sets interrupt to handle factory reset (pressing a button for 15 seconds)
  ///
  /// Factory resets deletes both the wifi credentials and the monitor server token
  void setup() noexcept;
}

#endif