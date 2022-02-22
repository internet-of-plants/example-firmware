#include "driver/thread.hpp"
#include "driver/device.hpp"
#include "Arduino.h"

namespace driver {
auto Thread::sleep(uint64_t ms) const noexcept -> void {
  ::delay(ms);
}
auto Thread::yield() const noexcept -> void {
  ::yield();
}
auto Thread::halt() const noexcept -> void {
  driver::device.deepSleep(0);
  this->abort();
}

auto Thread::abort() const noexcept -> void {
  __panic_func(__FILE__, __LINE__, __PRETTY_FUNCTION__);
}
auto Thread::now() const noexcept -> iop::time {
  return millis();
}
}