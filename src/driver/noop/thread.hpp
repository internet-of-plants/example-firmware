#include "driver/thread.hpp"

namespace driver {
auto Thread::sleep(iop::time::milliseconds ms) const noexcept -> void { (void) ms; }
auto Thread::yield() const noexcept -> void {}
auto Thread::abort() const noexcept -> void { while (true) {} }
auto Thread::timeRunning() const noexcept -> iop::time::milliseconds { static iop::time::milliseconds val = 0; return val++; }
}