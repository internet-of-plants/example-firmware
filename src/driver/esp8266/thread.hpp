#include "driver/thread.hpp"
#include "Arduino.h"

namespace driver {
auto Thread::sleep(iop::time::milliseconds ms) const noexcept -> void {
  ::delay(ms);
}
auto Thread::yield() const noexcept -> void {
  ::yield();
}
auto Thread::abort() const noexcept -> void {
  __panic_func(__FILE__, __LINE__, __PRETTY_FUNCTION__);
}
auto Thread::timeRunning() const noexcept -> iop::time::milliseconds {
  return millis();
}
}