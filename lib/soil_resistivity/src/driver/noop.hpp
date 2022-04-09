#ifndef IOP_LIB_SENSORS_SOIL_RESISTIVITY_NOOP_HPP
#define IOP_LIB_SENSORS_SOIL_RESISTIVITY_NOOP_HPP

#include <soil_resistivity.hpp>

namespace sensor {
SoilResistivity::SoilResistivity(const driver::io::Pin powerPin) noexcept { (void) powerPin; }

auto SoilResistivity::begin() noexcept -> void {}
auto SoilResistivity::measure() noexcept -> uint16_t { return 0; }

auto SoilResistivity::operator=(SoilResistivity && other) noexcept -> SoilResistivity & { (void) other; return *this; }
SoilResistivity::~SoilResistivity() noexcept {}
}

#endif