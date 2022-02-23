#ifndef IOP_STORAGE_HPP
#define IOP_STORAGE_HPP

#include "driver/log.hpp"
#include "utils.hpp"
#include <optional>

/// Wraps storage memory to provide a safe and ergonomic API
class Storage {
  iop::Log logger;

public:
  explicit Storage(iop::LogLevel logLevel) noexcept: logger(logLevel, IOP_STR("STORAGE")) {}

  /// Initializes storage memory storage
  static auto setup() noexcept -> void;

  auto token() const noexcept -> std::optional<std::reference_wrapper<const AuthToken>>;
  void removeToken() const noexcept;
  void setToken(const AuthToken &token) const noexcept;

  auto wifi() const noexcept -> std::optional<std::reference_wrapper<const WifiCredentials>>;
  void removeWifi() const noexcept;
  void setWifi(const WifiCredentials &config) const noexcept;
};

#ifndef IOP_STORAGE
#define IOP_STORAGE_DISABLED
#endif

#endif