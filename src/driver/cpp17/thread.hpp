#include "driver/thread.hpp"

#include <chrono>
#include <thread>

namespace driver {
void Thread::sleep(uint64_t ms) const noexcept {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
void Thread::yield() const noexcept {
  std::this_thread::yield();
}
void Thread::panic_() const noexcept {
  std::abort();
}

static const auto start = std::chrono::system_clock::now().time_since_epoch();
auto Thread::now() const noexcept -> iop::time {
  const auto epoch = (std::chrono::system_clock::now() - start).time_since_epoch();
  const auto time = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
  if (time < 0) return 0;
  return static_cast<iop::time>(time);
}
}