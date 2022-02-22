#include "driver/storage.hpp"
#include "driver/panic.hpp"

#include <fstream>

namespace driver {
// This driver is horrible, please fix this
// Use fopen, properly report errors, keep the file open, memmap (?)...

auto Storage::setup(uintmax_t size) noexcept -> bool {
    iop_assert(size > 0, IOP_STATIC_STR("Storage size is zero"));

    if (!this->buffer) {
        this->size = size;
        this->buffer = new (std::nothrow) uint8_t[size];
        if (!this->buffer) return false;
        std::memset(this->buffer, '\0', size);
    }

    std::ifstream file("eeprom.dat");
    if (!file.is_open()) return false;

    file.read((char*) buffer, static_cast<std::streamsize>(size));
    if (file.fail()) return false;

    file.close();
    return !file.fail();
}
auto Storage::commit() noexcept -> bool {
    // TODO: properly log errors
    iop_assert(this->buffer, IOP_STATIC_STR("Unable to allocate storage"));
    //iop::Log(logLevel, IOP_STATIC_STR("EEPROM")).debug(IOP_STATIC_STR("Commit: "), utils::base64Encode(this->storage.get(), this->size));
    std::ofstream file("eeprom.dat");
    if (!file.is_open()) return false;

    file.write((char*) buffer, static_cast<std::streamsize>(size));
    if (file.fail()) return false;

    file.close();
    return !file.fail();
}
auto Storage::asRef() const noexcept -> uint8_t const * {
    iop_assert(this->buffer, IOP_STATIC_STR("Allocation failed"));
    return this->buffer;
}
auto Storage::asMut() noexcept -> uint8_t * {
    iop_assert(this->buffer, IOP_STATIC_STR("Allocation failed"));
    return this->buffer;
}
}