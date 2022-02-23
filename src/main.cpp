#include "loop.hpp"

// TODO: log restart reason Esp::getResetInfoPtr()

namespace driver {
auto setup() noexcept -> void {
  panic::setup();
  network_logger::setup();
  eventLoop.setup();
}
auto loop() noexcept -> void { eventLoop.loop(); }
}