#ifndef IOP_LIB_SENSORS_SOIL_RESISTIVITY_ARDUINO_HPP
#define IOP_LIB_SENSORS_SOIL_RESISTIVITY_ARDUINO_HPP

#include <soil_resistivity.hpp>

#include <Arduino.h>
#undef HIGH
#undef LOW
#undef OUTPUT

#include <iop/panic.hpp>

namespace sensor {
#define SENSOR(self) static_cast<driver::io::Pin*>((self).sensor)

SoilResistivity::SoilResistivity(const driver::io::Pin powerPin) noexcept: sensor(new (std::nothrow) driver::io::Pin(powerPin)) {
    iop_assert(this->sensor != nullptr, IOP_STR("Unable to allocate SoilResistivity"));
}

auto SoilResistivity::begin() noexcept -> void {
    iop_assert(this->sensor != nullptr, IOP_STR("Sensor is nullptr"));
    driver::gpio.setMode(*SENSOR(*this), driver::io::Mode::OUTPUT);
}
auto SoilResistivity::measure() noexcept -> uint16_t {
    iop_assert(this->sensor != nullptr, IOP_STR("Sensor is nullptr"));
    digitalWrite(static_cast<uint8_t>(*SENSOR(*this)), static_cast<uint8_t>(driver::io::Data::HIGH));
    delay(2000); // NOLINT *-avoid-magic-numbers
    uint16_t value1 = analogRead(A0);
    delay(500); // NOLINT *-avoid-magic-numbers
    uint16_t value2 = analogRead(A0);
    delay(500); // NOLINT *-avoid-magic-numbers
    uint16_t value = (value1 + value2 + analogRead(A0)) / 3;
    digitalWrite(static_cast<uint8_t>(*SENSOR(*this)), static_cast<uint8_t>(driver::io::Data::LOW));
    return value;
}

auto SoilResistivity::operator=(SoilResistivity && other) noexcept -> SoilResistivity & {
    delete SENSOR(*this);
    this->sensor = other.sensor;
    other.sensor = nullptr;
    return *this;
}
SoilResistivity::~SoilResistivity() noexcept {
    delete SENSOR(*this);
}
}

#endif