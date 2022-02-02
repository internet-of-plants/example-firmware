#include "driver/storage.hpp"
#include "driver/panic.hpp"

namespace driver {
void Storage::setup(size_t size) noexcept {
    this->size = size;
    if (!buffer) {
        buffer = new (std::nothrow) uint8_t[size];
        memset(buffer, 0, size);
    }
}
void Storage::commit() noexcept {}
uint8_t const * Storage::asRef() const noexcept { if (!buffer) iop_panic(IOP_STATIC_STRING("Buffer is nullptr")); return buffer; }
uint8_t * Storage::asMut() noexcept { if (!buffer) iop_panic(IOP_STATIC_STRING("Buffer is nullptr")); return buffer; }
}