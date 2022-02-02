#include "driver/storage.hpp"
#include "driver/panic.hpp"
#include "EEPROM.h"

// TODO: move this to global structure
//EEPROMClass EEPROM;

namespace driver {
// TODO: properly handle EEPROM internal errors
void Storage::setup(size_t size) noexcept {
    this->size = size;
    EEPROM.begin(size);
}
void Storage::commit() noexcept {
    // TODO: report errors in storage usage
    iop_assert(EEPROM.commit(), IOP_STATIC_STRING("EEPROM commit failed"));
}
uint8_t const * Storage::asRef() const noexcept {
    return EEPROM.getConstDataPtr();
}
uint8_t * Storage::asMut() noexcept {
    return EEPROM.getDataPtr();
}
}