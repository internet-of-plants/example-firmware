#include "driver/main.hpp"
#include "driver/thread.hpp"

// Dumb way of supporting every currently supported environment with a single file

#ifdef ARDUINO
#include <Arduino.h>
#endif

void setup() {
    driver::setup();
}

void loop() {
    driver::loop();
}

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