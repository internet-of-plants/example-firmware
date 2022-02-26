#include "driver/storage.hpp"
#include "driver/panic.hpp"
#include "sys/pgmspace.h"
#include "EEPROM.h"

namespace driver {
auto Storage::setup(const uintmax_t size) noexcept -> bool {
    this->size = size;
    // Throws exception on OOM
    EEPROM.begin(size);
    return true;
}
auto Storage::commit() noexcept -> bool {
    return EEPROM.commit();
}
// Can never be nullptr as EEPROM.begin throws on OOM
auto Storage::asRef() const noexcept -> uint8_t const * {
    return EEPROM.getConstDataPtr();
}
auto Storage::asMut() noexcept -> uint8_t * {
    return EEPROM.getDataPtr();
}
}