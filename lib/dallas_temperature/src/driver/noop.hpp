#ifndef IOP_LIB_SENSORS_DALLAS_TEMP_NOOP_HPP
#define IOP_LIB_SENSORS_DALLAS_TEMP_NOOP_HPP

#include <dallas_temperature.hpp>

namespace dallas {
TemperatureCollection::TemperatureCollection(driver::io::Pin pin) noexcept { (void) pin; }

auto TemperatureCollection::begin() noexcept -> void {}
auto TemperatureCollection::measure() noexcept -> float { return 0.; }

auto TemperatureCollection::operator=(TemperatureCollection && other) noexcept -> TemperatureCollection & { (void) other; return *this; }
TemperatureCollection::~TemperatureCollection() noexcept {}
}

#endif