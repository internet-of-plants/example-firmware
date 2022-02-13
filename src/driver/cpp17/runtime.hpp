#include "driver/main.hpp"
#include "driver/thread.hpp"

int main(int argc, char** argv) {
  (void) argc;
  (void) argv;
  
  driver::setup();
  while (true) {
    driver::loop();
    driver::thisThread.sleep(100);
  }
  return 0;
}