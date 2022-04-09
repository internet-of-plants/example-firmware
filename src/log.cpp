#include "iop/log.hpp"
#include "configuration.hpp"
#include "loop.hpp"

#ifdef IOP_NETWORK_LOGGING

#include "iop/thread.hpp"

static auto staticPrinter(const iop::StaticString str, iop::LogLevel level, iop::LogType kind) noexcept -> void;
static auto viewPrinter(const std::string_view, iop::LogLevel level, iop::LogType kind) noexcept -> void;
static auto setuper(iop::LogLevel level) noexcept -> void;
static auto flusher() noexcept -> void;

static auto hook = iop::LogHook(viewPrinter, staticPrinter, setuper, flusher);

namespace network_logger {
  void setup() noexcept {
    iop::Log::setHook(hook);
    iop::Log::setup(config::logLevel);
  }
}

static auto currentLog = std::string();
static auto logToNetwork = true;

void reportLog() noexcept {
  if (!logToNetwork || !currentLog.length())
    return;
  logToNetwork = false;

  driver::thisThread.yield();

  const auto token = eventLoop.storage().token();
  if (token) {
    eventLoop.api().registerLog(*token, currentLog);
  } else {
    iop::Log(iop::LogLevel::WARN, IOP_STR("NETWORK LOGGING")).warn(IOP_STR("Unable to log to the monitor server, not authenticated"));
  }
  logToNetwork = true;
  currentLog.clear();
}

static void staticPrinter(const iop::StaticString str, const iop::LogLevel level, const iop::LogType kind) noexcept {
  iop::LogHook::defaultStaticPrinter(str, level, kind);

  if (logToNetwork && level >= iop::LogLevel::INFO) {
    currentLog += str.toString();

    if (kind == iop::LogType::END || kind == iop::LogType::STARTEND) {
      iop::LogHook::defaultStaticPrinter(IOP_STR("Logging to network\n"), iop::LogLevel::INFO, iop::LogType::STARTEND);
      reportLog();
      iop::LogHook::defaultStaticPrinter(IOP_STR("Logged to network\n"), iop::LogLevel::INFO, iop::LogType::STARTEND);
    }
  }
}
static auto viewPrinter(const std::string_view str, const iop::LogLevel level, const iop::LogType kind) noexcept -> void {
  iop::LogHook::defaultViewPrinter(str, level, kind);

  if (logToNetwork && level >= iop::LogLevel::INFO) {
    currentLog += str;

    if (kind == iop::LogType::END || kind == iop::LogType::STARTEND) {
      iop::LogHook::defaultStaticPrinter(IOP_STR("Logging to network\n"), iop::LogLevel::INFO, iop::LogType::STARTEND);
      reportLog();
      iop::LogHook::defaultStaticPrinter(IOP_STR("Logged to network\n"), iop::LogLevel::INFO, iop::LogType::STARTEND);
    }
  }
}
static auto flusher() noexcept -> void { iop::LogHook::defaultFlusher(); }
static auto setuper(iop::LogLevel level) noexcept -> void { iop::LogHook::defaultSetuper(level); }
#else
namespace network_logger {
  auto setup() noexcept -> void { iop::Log::setup(config::logLevel); }
}
#endif
