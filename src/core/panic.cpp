#include "core/panic.hpp"
#include "core/log.hpp"
#include "core/tracer.hpp"
#include "core/utils.hpp"
#include "core/lazy.hpp"

#include <iostream>

#include "driver/device.hpp"

static iop::Lazy<iop::Log> logger([]() { return iop::Log(iop::LogLevel::CRIT, F("PANIC")); });

static bool isPanicking = false;

const static iop::PanicHook defaultHook(iop::PanicHook::defaultViewPanic,
                                        iop::PanicHook::defaultStaticPanic,
                                        iop::PanicHook::defaultEntry,
                                        iop::PanicHook::defaultHalt);

static iop::PanicHook hook(defaultHook);

namespace iop {
void panicHandler(std::string_view msg, CodePoint const &point) noexcept {
  IOP_TRACE();
  hook.entry(msg, point);
  hook.viewPanic(msg, point);
  hook.halt(msg, point);
  std::abort();
  while (true) {
  } // Is this UB? It will trigger software watch-dog, but shouldn't be reached
}

#include <iostream>
void panicHandler(StaticString msg, CodePoint const &point) noexcept {
  IOP_TRACE();
  const auto msg_ = msg.toStdString();
  hook.entry(msg_, point);
  hook.staticPanic(msg, point);
  hook.halt(msg_, point);
  std::abort();
  while (true) {
  } // Is this UB? It will trigger software watch-dog, but shouldn't be reached
}
auto takePanicHook() noexcept -> PanicHook {
  auto old = hook;
  hook = defaultHook;
  return old;
}
void setPanicHook(PanicHook newHook) noexcept { hook = std::move(newHook); }

void PanicHook::defaultViewPanic(std::string_view const &msg,
                                 CodePoint const &point) noexcept {
  logger->crit(F("Line "), ::std::to_string(point.line()), F(" of file "), point.file(),
              F(" inside "), point.func(), F(": "), std::string(msg));
}
void PanicHook::defaultStaticPanic(iop::StaticString const &msg,
                                   CodePoint const &point) noexcept {
  logger->crit(F("Line "), ::std::to_string(point.line()), F(" of file "), point.file(),
              F(" inside "), std::string(point.func()), F(": "), msg);
}
void PanicHook::defaultEntry(std::string_view const &msg,
                             CodePoint const &point) noexcept {
  IOP_TRACE();
  if (isPanicking) {
    logger->crit(F("PANICK REENTRY: Line "), std::to_string(point.line()),
                F(" of file "), point.file(), F(" inside "), std::string(point.func()),
                F(": "), std::string(msg));
    iop::logMemory(*logger);
    ESP.deepSleep(0);
    // std::string_view may be non-zero terminated
    __panic_func(point.file().asCharPtr(), point.line(), std::string(point.func()).c_str());
  }
  isPanicking = true;

  constexpr const uint16_t oneSecond = 1000;
  delay(oneSecond);
}
void PanicHook::defaultHalt(std::string_view const &msg,
                            CodePoint const &point) noexcept {
  (void)msg;
  IOP_TRACE();
  ESP.deepSleep(0);
  // std::string_view may be non-zero terminated
  __panic_func(point.file().asCharPtr(), point.line(), std::string(point.func()).c_str());
}
} // namespace iop