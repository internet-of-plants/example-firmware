#ifdef IOP_POSIX
#include "driver/cpp17/storage.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/storage.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/storage.hpp"
#elif defined(IOP_ESP32)
#include "driver/esp32/storage.hpp"
#else
#error "Target not supported"
#endif

namespace driver {
Storage storage;

auto Storage::read(const uintmax_t address) const noexcept -> std::optional<uint8_t> {
    if (address >= this->size) return std::optional<uint8_t>();
    return this->asRef()[address];
}

auto Storage::write(const uintmax_t address, uint8_t const val) noexcept -> bool {
    iop_assert(this->buffer, IOP_STATIC_STR("Buffer is nullptr"));
    if (address >= this->size) return false;
    this->asMut()[address] = val;
    return true;
}
}