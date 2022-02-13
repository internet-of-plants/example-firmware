#include "driver/runtime.hpp"
#include "driver/thread.hpp"
#include "driver/panic.hpp"

// POSIX
#include <sys/resource.h>

static char * filename;
static uintptr_t stackstart = 0;
namespace driver {
std::string_view execution_path() noexcept {
  iop_assert(filename != nullptr, IOP_STATIC_STRING("Filename wasn't initialized, maybe you are trying to get the execution path before main runs?"));
  return filename;
}
uintmax_t stack_size() noexcept {
  uintptr_t stackend;
  stackend = (uintptr_t) (void*) &stackend;

  // POSIX
  struct rlimit limit;
  getrlimit(RLIMIT_STACK, &limit);

  iop_assert(stackstart != 0, IOP_STATIC_STRING("The stack start has never been collected, maybe are you trying to get the stack size before main runs?"));
  return limit.rlim_cur - (stackstart - stackend);
}
}

int main(int argc, char** argv) {
  stackstart = (uintptr_t) (void*) &argc;
  iop_assert(argc > 0, IOP_STATIC_STRING("argc is 0"));
  filename = argv[0];

  driver::setup();
  while (true) {
    driver::loop();
    driver::thisThread.sleep(100);
  }
  return 0;
}