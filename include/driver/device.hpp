#ifndef IOP_DRIVER_DEVICE_HPP
#define IOP_DRIVER_DEVICE_HPP

#include "driver/string.hpp"
#include <stdint.h>
#include <array>

class HeapSelectIram;
class HeapSelectDram;

namespace driver {
class HeapSelectIram {
  ::HeapSelectIram *ptr;
public:
  HeapSelectIram() noexcept;
  ~HeapSelectIram() noexcept;
};
class HeapSelectDram {
  ::HeapSelectDram *ptr;
public:
  HeapSelectDram() noexcept;
  ~HeapSelectDram() noexcept;
};

class Device {
public:
  void syncNTP() const noexcept;
  auto availableSpace() const noexcept -> uintmax_t;
  auto availableStack() const noexcept -> uintmax_t;
  auto availableHeap() const noexcept -> uintmax_t;
  auto vcc() const noexcept -> uint16_t;
  auto biggestHeapBlock() const noexcept -> uintmax_t;
  void deepSleep(size_t seconds) const noexcept;
  std::array<char, 32>& firmwareMD5() const noexcept;
  std::array<char, 17>& macAddress() const noexcept;
  ::iop::StaticString platform() const noexcept;
};
extern Device device;
}

#endif