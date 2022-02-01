#include "driver/main.hpp"
#include "driver/thread.hpp"

namespace driver {
char *filename = nullptr;
}

int main(int argc, char** argv) {
  // We store the execution path to be able to update its own binary
  if (argc > 0) {
    driver::filename = argv[0];
  }

  driver::setup();
  while (true) {
    driver::loop();
    driver::thisThread.sleep(100);
  }
  return 0;
}