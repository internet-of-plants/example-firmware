#ifndef IOP_LIB_SENSORS_DHT_HPP
#define IOP_LIB_SENSORS_DHT_HPP

#include <iop/io.hpp>
#include <stdint.h>

namespace dht {
class Dht {
    void *sensor;
public:
    Dht(driver::io::Pin pin, uint8_t dhtVersion) noexcept;
    auto begin() noexcept -> void;
    auto measureTemperature() noexcept -> float;
    auto measureHumidity() noexcept -> float;
    auto measureHeatIndex() noexcept -> float;

    auto operator=(Dht && other) noexcept -> Dht &;
    ~Dht() noexcept;

    Dht(Dht && other) noexcept: sensor(other.sensor) {
        other.sensor = nullptr;
    }
    Dht(const Dht & other) noexcept = delete;
    auto operator=(const Dht & other) noexcept = delete;
};
}

#endif