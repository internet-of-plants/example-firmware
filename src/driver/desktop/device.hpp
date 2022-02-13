#include "driver/device.hpp"
#include <stdint.h>
#include <thread>
#include <filesystem>
#include <fstream>

#include <iostream>

namespace driver {
void Device::syncNTP() const noexcept {}
auto Device::vcc() const noexcept -> uint16_t { return 1; }

auto Device::platform() const noexcept -> iop::StaticString { return IOP_STATIC_STRING("DESKTOP"); }
auto Device::availableStorage() const noexcept -> uintmax_t {
  // TODO: handle errors
  std::error_code code;
  auto available = std::filesystem::space(std::filesystem::current_path(), code).available;
  if (code) return 0;
  return available;
}
auto Device::availableMemory() const noexcept -> Memory {
  std::map<std::string_view, uintmax_t> heap;
  heap.insert({ std::string_view("RAM"), static_cast<uintmax_t>(1) });

  std::map<std::string_view, uintmax_t> biggestBlock;
  biggestBlock.insert({ std::string_view("RAM"), static_cast<uintmax_t>(1) });

  std::cout << &biggestBlock << std::endl;
  return Memory(1, heap, biggestBlock);
}
void Device::deepSleep(size_t seconds) const noexcept {
  if (seconds == 0) seconds = SIZE_MAX;
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
}
iop::MD5Hash & Device::firmwareMD5() const noexcept {
  static iop::MD5Hash hash;
  static bool cached = false;
  if (cached)
    return hash;
  //std::ifstream file(std::filesystem::current_path());
  hash.fill('A');
  cached = true;
  return hash;
}
iop::MacAddress & Device::macAddress() const noexcept {
  static iop::MacAddress mac;
  mac.fill('A');
  return mac;
}
}