#include "driver/thread.hpp"

#include <chrono>
#include <thread>

namespace driver {
auto Thread::sleep(iop::time::milliseconds ms) const noexcept -> void {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
auto Thread::yield() const noexcept -> void {
  std::this_thread::yield();
}
auto Thread::abort() const noexcept -> void {
  std::abort();
}
auto Thread::halt() const noexcept -> void {
  this->abort();
}

static const auto start = std::chrono::system_clock::now().time_since_epoch();
auto Thread::timeRunning() const noexcept -> iop::time::milliseconds {
  const auto epoch = (std::chrono::system_clock::now() - start).time_since_epoch();
  const auto time = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
  if (time < 0) return 0;
  return static_cast<iop::time::milliseconds>(time);
}
}