#include "loop.hpp"

// TODO: log restart reason Esp::getResetInfoPtr()

GlobalData globalData;
namespace driver {
void setup() noexcept {
  panic::setup();
  network_logger::setup();
  eventLoop.setup();
}
void loop() noexcept { eventLoop.loop(); }
}