#ifdef IOP_FACTORY_RESET
#include "iop/device.hpp"
#include "iop/io.hpp"
#include "iop/thread.hpp"
#include "configuration.hpp"

static volatile iop::time::milliseconds resetStateTime = 0;

void IOP_RAM buttonChanged() noexcept {
  IOP_TRACE();

  constexpr const uint32_t fifteenSeconds = 15000;
  if (driver::gpio.digitalRead(config::factoryResetButton) == driver::io::Data::HIGH) {
    resetStateTime = driver::thisThread.timeRunning();
    
    if (config::logLevel >= iop::LogLevel::INFO)
      iop::Log::print("[INFO] RESET: Press FACTORY_RESET button for 15 seconds more to factory reset the device\n", iop::LogLevel ::INFO, iop::LogType::STARTEND);
  } else if (resetStateTime + fifteenSeconds < driver::thisThread.timeRunning()) {
      utils::scheduleInterrupt(InterruptEvent::FACTORY_RESET);

      if (config::logLevel >= iop::LogLevel::INFO)
        iop::Log::print("[INFO] RESET: Factory reset scheduled, it will run in the next loop run\n", iop::LogLevel::INFO, iop::LogType::STARTEND);
  }
}

namespace reset {
void setup() noexcept {
  IOP_TRACE();
  driver::gpio.setMode(config::factoryResetButton, driver::io::Mode::INPUT);
  driver::gpio.setInterruptCallback(config::factoryResetButton, driver::io::InterruptState::CHANGE, buttonChanged);
}
} // namespace reset
#else
#include "iop/log.hpp"
namespace reset {
void setup() noexcept { IOP_TRACE(); }
} // namespace reset
#endif