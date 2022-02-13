#include "driver/device.hpp"

namespace driver {
void Device::syncNTP() const noexcept {}
auto Device::platform() const noexcept -> ::iop::StaticString { return IOP_STATIC_STRING("NOOP"); }
auto Device::vcc() const noexcept -> uint16_t { return UINT16_MAX; }
auto Device::availableStorage() const noexcept -> uintmax_t { return 1000000; }
auto Device::availableMemory() const noexcept -> Memory {
    std::map<std::string_view, uintmax_t> heap;
    heap.insert({ std::string_view("DRAM"), 20000 });

    std::map<std::string_view, uintmax_t> biggestBlock;
    biggestBlock.insert({ std::string_view("DRAM"), 20000 });

    return Memory {
        .availableStack = 2000,
        .availableHeap = heap,
        .biggestHeapBlock = biggestBlock,
    };
}
void Device::deepSleep(const size_t seconds) const noexcept { (void) seconds; }
iop::MD5Hash & Device::firmwareMD5() const noexcept { static iop::MD5Hash hash; hash.fill('\0'); hash = { 'M', 'D', '5' }; return hash; }
iop::MacAddress & Device::macAddress() const noexcept { static iop::MacAddress mac; mac.fill('\0'); mac = { 'M', 'A', 'C' }; return mac; }
}