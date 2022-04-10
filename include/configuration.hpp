#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include "iop-hal/io.hpp"

namespace config {
constexpr static iop_hal::io::Pin soilTemperature = iop_hal::io::Pin::D5;
constexpr static iop_hal::io::Pin airTempAndHumidity = iop_hal::io::Pin::D6;
constexpr static iop_hal::io::Pin soilResistivityPower = iop_hal::io::Pin::D7;
constexpr static iop_hal::io::Pin factoryResetButton = iop_hal::io::Pin::D1;

/// Version of DHT (Digital Humidity and Temperature) sensor. (ex: DHT11 or DHT21 or DHT22...)
constexpr static uint8_t dhtVersion = 22; // DHT22
}

#endif