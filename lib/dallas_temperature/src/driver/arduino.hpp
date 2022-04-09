#ifndef IOP_LIB_SENSORS_DALLAS_TEMP_ARDUINO_HPP
#define IOP_LIB_SENSORS_DALLAS_TEMP_ARDUINO_HPP

#include <dallas_temperature.hpp>
#include <iop/panic.hpp>

#include <OneWire.h>
#include <DallasTemperature.h>

namespace dallas {
#define SENSOR(self) static_cast<DallasTemperature*>((self).sensor)

/// Be ware, self-referential struct, it should _never_ move addresses.
class OneWireBus {
    OneWire oneWire;
    DallasTemperature temp;

public:
    OneWireBus(driver::io::Pin pin) noexcept: oneWire(static_cast<uint8_t>(pin)), temp(&oneWire) {}

    OneWireBus(const OneWireBus &other) noexcept = delete;
    OneWireBus(OneWireBus &&other) noexcept = delete;
    auto operator=(const OneWireBus &other) noexcept -> OneWireBus & = delete;
    auto operator=(OneWireBus &&other) noexcept -> OneWireBus & = delete;

    friend TemperatureCollection;
};

TemperatureCollection::TemperatureCollection(driver::io::Pin pin) noexcept: sensor(new (std::nothrow) OneWireBus(pin)) {
    iop_assert(this->sensor != nullptr, IOP_STR("Unable to allocate OneWireBus"));
}

auto TemperatureCollection::begin() noexcept -> void {
    iop_assert(this->sensor != nullptr, IOP_STR("Sensor is nullptr"));
    SENSOR(*this)->begin();
}
auto TemperatureCollection::measure() noexcept -> float {
    iop_assert(this->sensor != nullptr, IOP_STR("Sensor is nullptr"));
    // Blocks until reading is done
    SENSOR(*this)->requestTemperatures();
    // Accessing by index is bad. It's slow, we should store the sensor's address
    // and use it
    return SENSOR(*this)->getTempCByIndex(0);
}

auto TemperatureCollection::operator=(TemperatureCollection && other) noexcept -> TemperatureCollection & {
    delete SENSOR(*this);
    this->sensor = other.sensor;
    other.sensor = nullptr;
    return *this;
}
TemperatureCollection::~TemperatureCollection() noexcept {
    delete SENSOR(*this);
}
}

#endif