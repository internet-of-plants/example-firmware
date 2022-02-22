#ifndef IOP_SERIAL
#include "driver/noop/log.hpp"
#elif defined(IOP_POSIX)
#include "driver/cpp17/log.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/log.hpp"
#elif defined(IOP_NOOP)
#ifdef ARDUINO
#include "driver/esp8266/log.hpp"
#else
#include "driver/cpp17/log.hpp"
#endif
#elif defined(IOP_ESP32)
#include "driver/esp32/log.hpp"
#else
#error "Target not supported"
#endif

#include "driver/network.hpp"
#include "driver/device.hpp"
#include "driver/wifi.hpp"

static bool initialized = false;
static bool isTracing_ = false; 
static bool shouldFlush_ = true;

void iop::Log::shouldFlush(const bool flush) noexcept {
  shouldFlush_ = flush;
}

auto iop::Log::isTracing() noexcept -> bool { 
  return isTracing_;
}

constexpr static iop::LogHook defaultHook(iop::LogHook::defaultViewPrinter,
                                      iop::LogHook::defaultStaticPrinter,
                                      iop::LogHook::defaultSetuper,
                                      iop::LogHook::defaultFlusher);
static iop::LogHook hook = defaultHook;

namespace iop {
void IOP_RAM Log::setup(LogLevel level) noexcept { hook.setup(level); }
void Log::flush() noexcept { if (shouldFlush_) hook.flush(); }
void IOP_RAM Log::print(const std::string_view view, const LogLevel level,
                                const LogType kind) noexcept {
  if (level > LogLevel::TRACE)
    hook.viewPrint(view, level, kind);
  else
    hook.traceViewPrint(view, level, kind);
}
void IOP_RAM Log::print(const StaticString progmem,
                                const LogLevel level,
                                const LogType kind) noexcept {
  if (level > LogLevel::TRACE)
    hook.staticPrint(progmem, level, kind);
  else
    hook.traceStaticPrint(progmem, level, kind);
}
auto Log::takeHook() noexcept -> LogHook {
  initialized = false;
  auto old = hook;
  hook = defaultHook;
  return old;
}
void Log::setHook(LogHook newHook) noexcept {
  initialized = false;
  hook = std::move(newHook);
}

void Log::printLogType(const LogType &logType,
                       const LogLevel &level) const noexcept {
  if (level == LogLevel::NO_LOG)
    return;

  switch (logType) {
  case LogType::CONTINUITY:
  case LogType::END:
    break;

  case LogType::START:
  case LogType::STARTEND:
    Log::print(IOP_STATIC_STR("["), level, LogType::START);
    Log::print(this->levelToString(level).get(), level, LogType::CONTINUITY);
    Log::print(IOP_STATIC_STR("] "), level, LogType::CONTINUITY);
    Log::print(this->target_.get(), level, LogType::CONTINUITY);
    Log::print(IOP_STATIC_STR(": "), level, LogType::CONTINUITY);
  };
}

void Log::log(const LogLevel &level, const StaticString &msg,
              const LogType &logType,
              const StaticString &lineTermination) const noexcept {
  if (this->level_ > level)
    return;

  Log::flush();
  this->printLogType(logType, level);
  Log::print(msg, level, LogType::CONTINUITY);
  if (logType == LogType::END || logType == LogType::STARTEND) {
    Log::print(lineTermination, level, LogType::END);
  } else if (lineTermination.length() != 0) {
    Log::print(lineTermination, level, LogType::CONTINUITY);
  }
  Log::flush();
}

void Log::log(const LogLevel &level, const std::string_view &msg,
              const LogType &logType,
              const StaticString &lineTermination) const noexcept {
  if (this->level_ > level)
    return;

  Log::flush();
  this->printLogType(logType, level);
  Log::print(msg, level, LogType::CONTINUITY);
  if (logType == LogType::END || logType == LogType::STARTEND) {
    Log::print(lineTermination, level, LogType::END);
  } else if (lineTermination.length() != 0) {
    Log::print(lineTermination, level, LogType::CONTINUITY);
  }
  Log::flush();
}

auto Log::levelToString(const LogLevel level) const noexcept -> StaticString {
  switch (level) {
  case LogLevel::TRACE:
    return IOP_STATIC_STR("TRACE");
  case LogLevel::DEBUG:
    return IOP_STATIC_STR("DEBUG");
  case LogLevel::INFO:
    return IOP_STATIC_STR("INFO");
  case LogLevel::WARN:
    return IOP_STATIC_STR("WARN");
  case LogLevel::ERROR:
    return IOP_STATIC_STR("ERROR");
  case LogLevel::CRIT:
    return IOP_STATIC_STR("CRIT");
  case LogLevel::NO_LOG:
    return IOP_STATIC_STR("NO_LOG");
  }
  return IOP_STATIC_STR("UNKNOWN");
}

void IOP_RAM LogHook::defaultStaticPrinter(
    const StaticString str, const LogLevel level, const LogType type) noexcept {
#ifdef IOP_SERIAL
  driver::logPrint(str);
#else
  (void)str;
#endif
  (void)type;
  (void)level;
}
void IOP_RAM
LogHook::defaultViewPrinter(const std::string_view str, const LogLevel level, const LogType type) noexcept {
#ifdef IOP_SERIAL
  driver::logPrint(str);
#else
  (void)str;
#endif
  (void)type;
  (void)level;
}
void IOP_RAM
LogHook::defaultSetuper(const LogLevel level) noexcept {
  isTracing_ |= level == LogLevel::TRACE;
  static bool hasInitialized = false;
  const auto shouldInitialize = !hasInitialized;
  hasInitialized = true;
  if (shouldInitialize)
    driver::logSetup(level);
}
void LogHook::defaultFlusher() noexcept {
#ifdef IOP_SERIAL
  driver::logFlush();
#endif
}
// NOLINTNEXTLINE *-use-equals-default
LogHook::LogHook(LogHook const &other) noexcept
    : viewPrint(other.viewPrint), staticPrint(other.staticPrint),
      setup(other.setup), flush(other.flush),
      traceViewPrint(other.traceViewPrint),
      traceStaticPrint(other.traceStaticPrint) {}
LogHook::LogHook(LogHook &&other) noexcept
    // NOLINTNEXTLINE cert-oop11-cpp cert-oop54-cpp *-move-constructor-init
    : viewPrint(other.viewPrint), staticPrint(other.staticPrint),
      setup(other.setup), flush(other.flush),
      traceViewPrint(other.traceViewPrint),
      traceStaticPrint(other.traceStaticPrint) {}
auto LogHook::operator=(LogHook const &other) noexcept -> LogHook & {
  if (this == &other)
    return *this;
  this->viewPrint = other.viewPrint;
  this->staticPrint = other.staticPrint;
  this->setup = other.setup;
  this->flush = other.flush;
  this->traceViewPrint = other.traceViewPrint;
  this->traceStaticPrint = other.traceStaticPrint;
  return *this;
}
auto LogHook::operator=(LogHook &&other) noexcept -> LogHook & {
  *this = other;
  return *this;
}

Tracer::Tracer(CodePoint point) noexcept : point(std::move(point)) {
  if (!Log::isTracing())
    return;

  Log::flush();
  Log::print(IOP_STATIC_STR("[TRACE] TRACER: Entering new scope, at line "), LogLevel::TRACE, LogType::START);
  Log::print(std::to_string(this->point.line()), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(IOP_STATIC_STR(", in function "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.func(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(IOP_STATIC_STR(", at file "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.file(), LogLevel::TRACE, LogType::CONTINUITY);

  {
    // Could this cause memory fragmentation?
    auto memory = driver::device.availableMemory();
    
    Log::print(IOP_STATIC_STR("\n[TRACE] TRACER: Free Stack "), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(std::to_string(memory.availableStack), LogLevel::TRACE, LogType::CONTINUITY);

    for (auto& item: memory.availableHeap) {
      Log::print(IOP_STATIC_STR(", Free "), LogLevel::TRACE, LogType::CONTINUITY);
      Log::print(item.first, LogLevel::TRACE, LogType::CONTINUITY);
      Log::print(IOP_STATIC_STR(" "), LogLevel::TRACE, LogType::CONTINUITY);
      Log::print(std::to_string(item.second), LogLevel::TRACE, LogType::CONTINUITY);
    }
    for (auto& item: memory.biggestHeapBlock) {
      Log::print(IOP_STATIC_STR(", Biggest "), LogLevel::TRACE, LogType::CONTINUITY);
      Log::print(item.first, LogLevel::TRACE, LogType::CONTINUITY);
      Log::print(IOP_STATIC_STR(" Block "), LogLevel::TRACE, LogType::CONTINUITY);
      Log::print(std::to_string(item.second), LogLevel::TRACE, LogType::CONTINUITY);
    }
  }

  Log::print(IOP_STATIC_STR(", Connection "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(std::to_string(iop::data.wifi.status() == driver::StationStatus::GOT_IP), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(IOP_STATIC_STR("\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}
Tracer::~Tracer() noexcept {
  if (!Log::isTracing())
    return;

  Log::flush();
  Log::print(IOP_STATIC_STR("[TRACE] TRACER: Leaving scope, at line "), LogLevel::TRACE, LogType::START);
  Log::print(std::to_string(this->point.line()), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(IOP_STATIC_STR(", in function "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.func(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(IOP_STATIC_STR(", at file "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.file(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(IOP_STATIC_STR("\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}

void logMemory(const Log &logger) noexcept {
  if (logger.level() > LogLevel::INFO) return;

  Log::flush();
  Log::print(IOP_STATIC_STR("[INFO] "), logger.level(), LogType::START);
  Log::print(logger.target(), logger.level(), LogType::CONTINUITY);

  {
    // Could this cause memory fragmentation?
    auto memory = driver::device.availableMemory();

    Log::print(IOP_STATIC_STR(": Free Stack "), logger.level(), LogType::CONTINUITY);
    Log::print(std::to_string(memory.availableStack), logger.level(), LogType::CONTINUITY);

    for (auto& item: memory.availableHeap) {
      Log::print(IOP_STATIC_STR(", Free "), logger.level(), LogType::CONTINUITY);
      Log::print(item.first, logger.level(), LogType::CONTINUITY);
      Log::print(IOP_STATIC_STR(" "), LogLevel::TRACE, LogType::CONTINUITY);
      Log::print(std::to_string(item.second), logger.level(), LogType::CONTINUITY);
    }
    for (auto& item: memory.biggestHeapBlock) {
      Log::print(IOP_STATIC_STR(", Biggest "), logger.level(), LogType::CONTINUITY);
      Log::print(item.first, logger.level(), LogType::CONTINUITY);
      Log::print(IOP_STATIC_STR(" Block "), logger.level(), LogType::CONTINUITY);
      Log::print(std::to_string(item.second), logger.level(), LogType::CONTINUITY);
    }
  }
  Log::print(IOP_STATIC_STR(", Connection "), logger.level(), LogType::CONTINUITY);
  Log::print(std::to_string(iop::data.wifi.status() == driver::StationStatus::GOT_IP), logger.level(), LogType::CONTINUITY);
  Log::print(IOP_STATIC_STR("\n"), logger.level(), LogType::END);
  Log::flush();
}
} // namespace iop