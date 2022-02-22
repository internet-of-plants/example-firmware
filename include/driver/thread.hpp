#ifndef IOP_DRIVER_THREAD
#define IOP_DRIVER_THREAD

#include <stdint.h>

namespace iop {
using time = uintmax_t;
}

namespace driver {
class Thread {
public:
  auto now() const noexcept -> iop::time;
  void sleep(uint64_t ms) const noexcept;
  void yield() const noexcept;
  void abort() const noexcept __attribute__((noreturn));
  void halt() const noexcept __attribute__((noreturn));
};

extern Thread thisThread;
}

#endif