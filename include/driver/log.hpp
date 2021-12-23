#ifndef IOP_DRIVER_LOG_HPP
#define IOP_DRIVER_LOG_HPP

#include "core/string.hpp"

namespace iop {
enum class LogLevel;
}

void logSetup(const iop::LogLevel &level) noexcept;
void logPrint(const std::string_view msg) noexcept;
void logPrint(const iop::StaticString msg) noexcept;
void logFlush() noexcept;

#endif