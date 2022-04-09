#ifndef IOP_SENSORS_HPP
#define IOP_SENSORS_HPP

#include "iop/io.hpp"
#include "utils.hpp"

#include <dallas_temperature.hpp>
#include <dht.hpp>
#include <soil_resistivity.hpp>

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

  Sensors(const driver::io::Pin soilResistivityPower,
          const driver::io::Pin soilTemperature, const driver::io::Pin dht,
          const uint8_t dhtVersion) noexcept;

  Sensors(Sensors const &other) noexcept = delete;
  Sensors(Sensors &&other) noexcept = default;
  auto operator=(Sensors const &other) noexcept -> Sensors & = delete;
  auto operator=(Sensors &&other) noexcept -> Sensors & = default;
};

#endif