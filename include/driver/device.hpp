#ifndef IOP_DRIVER_DEVICE_HPP
#define IOP_DRIVER_DEVICE_HPP

#include "driver/string.hpp"
#include <stdint.h>
#include <array>
#include <map>

namespace driver {
struct Memory {
  uintmax_t availableStack;
  // It's a map because some environments have specialized RAMs
  std::map<std::string_view, uintmax_t> availableHeap;
  std::map<std::string_view, uintmax_t> biggestHeapBlock;

  Memory(uintmax_t stack, std::map<std::string_view, uintmax_t> heap, std::map<std::string_view, uintmax_t> biggestBlock) noexcept:
    availableStack(stack), availableHeap(heap), biggestHeapBlock(biggestBlock) {}
};

class Device {
public:
  void syncNTP() const noexcept;
  // Note: We handle firmware space and data space differently across environments, this could be improved
  auto availableStorage() const noexcept -> uintmax_t;
  auto availableMemory() const noexcept -> Memory;
  auto vcc() const noexcept -> uint16_t;
  void deepSleep(size_t seconds) const noexcept;
  std::array<char, 32>& firmwareMD5() const noexcept;
  std::array<char, 17>& macAddress() const noexcept;
  ::iop::StaticString platform() const noexcept;
};
extern Device device;
}

#endif