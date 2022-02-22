#include "driver/thread.hpp"

namespace driver {
auto Thread::sleep(uint64_t ms) const noexcept -> void { (void) ms; }
auto Thread::yield() const noexcept -> void {}
auto Thread::abort() const noexcept -> void { while (true) {} }
auto Thread::halt() const noexcept -> void { this->abort() }
auto Thread::now() const noexcept -> iop::time { return 0; }
}