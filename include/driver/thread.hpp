#ifndef IOP_DRIVER_THREAD
#define IOP_DRIVER_THREAD

#include <stdint.h>

namespace iop {
namespace time {
using milliseconds = uintmax_t;
}
}

namespace driver {
class Thread {
public:
  /// Returns numbers of milliseconds since boot.
  auto timeRunning() const noexcept -> iop::time::milliseconds;

  /// Stops device for number of specified milliseconds
  void sleep(iop::time::milliseconds ms) const noexcept;

  /// Allows background task to proceed. Needs to be done every few hundred milliseconds
  /// Since we are based on cooperative concurrency the running task needs to allow others to run
  /// Generally used to allow WiFi to be operated, for example, but supports any kind of multithread system
  void yield() const noexcept;

  /// Stops firmware execution with failure. Might result in a device restart, or not.
  /// Don't expect to come back from it.
  void abort() const noexcept __attribute__((noreturn));

  /// Pauses execution and never comes back.
  void halt() const noexcept __attribute__((noreturn));
};

extern Thread thisThread;
}

#endif