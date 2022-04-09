#ifndef IOP_LIB_SENSORS_SOIL_RESISTIVITY_HPP
#define IOP_LIB_SENSORS_SOIL_RESISTIVITY_HPP

#include <iop/io.hpp>
#include <stdint.h>

namespace sensor {
class SoilResistivity {
    void *sensor;
public:
    SoilResistivity(driver::io::Pin powerPin) noexcept;
    auto begin() noexcept -> void;
    auto measure() noexcept -> uint16_t;

    auto operator=(SoilResistivity && other) noexcept -> SoilResistivity &;
    ~SoilResistivity() noexcept;

    SoilResistivity(SoilResistivity && other) noexcept: sensor(other.sensor) {
        other.sensor = nullptr;
    }
    SoilResistivity(const SoilResistivity & other) noexcept = delete;
    auto operator=(const SoilResistivity & other) noexcept = delete;
};
}

#endif