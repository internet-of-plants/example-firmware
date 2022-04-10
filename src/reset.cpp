#include "iop-hal/device.hpp"
#include "iop-hal/io.hpp"
#include "iop-hal/thread.hpp"
#include "iop-hal/log.hpp"
#include "iop/utils.hpp"
#include "sensors.hpp"
#include "configuration.hpp"

static volatile iop::time::milliseconds resetStateTime = 0;
constexpr const uint32_t fifteenSeconds = 15000;

void IOP_RAM buttonChanged() noexcept {
  IOP_TRACE();

  if (iop_hal::gpio.digitalRead(config::factoryResetButton) == iop_hal::io::Data::HIGH) {
    resetStateTime = iop_hal::thisThread.timeRunning() + fifteenSeconds;
    
    if (IOP_LOG_LEVEL >= iop::LogLevel::INFO) {
      iop::Log::print("[INFO] RESET: Press this button for 15 seconds more to factory reset the device\n", iop::LogLevel::INFO, iop::LogType::STARTEND);
    }
  } else if (resetStateTime < iop_hal::thisThread.timeRunning()) {
    scheduledFactoryReset = true;

    if (IOP_LOG_LEVEL >= iop::LogLevel::INFO) {
      iop::Log::print("[INFO] RESET: Factory reset scheduled, it will run in the next loop run\n", iop::LogLevel::INFO, iop::LogType::STARTEND);
    }
  }
}

namespace reset {
void setup() noexcept {
  IOP_TRACE();
  iop_hal::gpio.setMode(config::factoryResetButton, iop_hal::io::Mode::INPUT);
  iop_hal::gpio.setInterruptCallback(config::factoryResetButton, iop_hal::io::InterruptState::CHANGE, buttonChanged);
}
} // namespace reset