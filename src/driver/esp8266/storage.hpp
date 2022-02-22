#include "driver/storage.hpp"
#include "driver/panic.hpp"
#include "sys/pgmspace.h"
#include "EEPROM.h"

// TODO: move this to global structure
//EEPROMClass EEPROM;

namespace driver {
// TODO: properly handle EEPROM internal errors
auto Storage::setup(const uintmax_t size) noexcept -> bool {
    this->size = size;
    EEPROM.begin(size);
    return true;
}
auto Storage::commit() noexcept -> bool {
    return EEPROM.commit();
}
auto Storage::asRef() const noexcept -> uint8_t const * {
    return EEPROM.getConstDataPtr();
}
auto Storage::asMut() noexcept -> uint8_t * {
    return EEPROM.getDataPtr();
}
}