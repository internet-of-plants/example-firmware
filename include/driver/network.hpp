#ifndef IOP_DRIVER_NETWORK_HPP
#define IOP_DRIVER_NETWORK_HPP

#include "driver/client.hpp"
#include "driver/wifi.hpp"
#include "driver/cert_store.hpp"
#include "driver/log.hpp"

#include <vector>

namespace iop {
extern driver::Wifi wifi;

/// Higher level error reporting. Lower level is handled by core
enum class NetworkStatus {
  IO_ERROR,
  BROKEN_SERVER,
  BROKEN_CLIENT,

  OK,
  FORBIDDEN,
};

/// Hook to schedule the next firmware binary update (_must not_ actually update, but only use it to schedule an update for the next loop run)
///
/// It's called by `iop::Network` whenever the server sets the `LAST_VERSION` HTTP header, to a value that isn't the MD5 of the current firmware binary.
class UpgradeHook {
public:
  using UpgradeScheduler = void (*) ();
  UpgradeScheduler schedule;

  constexpr explicit UpgradeHook(UpgradeScheduler scheduler) noexcept : schedule(scheduler) {}

  /// No-Op
  static void defaultHook() noexcept;
};


class Response {
  NetworkStatus status_;
  std::optional<std::vector<uint8_t>> payload_;
public:
  auto status() const noexcept -> NetworkStatus;
  auto payload() const noexcept -> std::optional<std::string_view>;
  explicit Response(const NetworkStatus &status) noexcept;
  Response(const NetworkStatus &status, std::vector<uint8_t> payload) noexcept;
};

enum class HttpMethod {
  GET,
  HEAD,
  POST,
  PUT,
  PATCH,
  DELETE,
  CONNECT,
  OPTIONS,
};

/// General lower level HTTP(S) client, that is focused on our need.
/// Which are security, good error reporting, no UB possible and ergonomy, in order.
///
/// Higher level than the simple ESP8266 client network abstractions, but still focused on the HTTP(S) protocol
///
/// _Must call_ `iop::Network::setCertStore` otherwise TLS won't work. It _will_ panic.
///
/// `iop::Network::setUpgradeHook` to set the hook that is called to schedule updates.
class Network {
  Log logger_;
  StaticString uri_;

  /// Sends a custom HTTP request that may be authenticated to the monitor server (primitive used by higher level methods)
  auto httpRequest(HttpMethod method, const std::optional<std::string_view> &token, StaticString path, const std::optional<std::string_view> &data) const noexcept -> std::variant<Response, int>;
public:
  Network(StaticString uri, const LogLevel &logLevel) noexcept;

  auto setup() const noexcept -> void;
  auto uri() const noexcept -> StaticString { return this->uri_; };

  /// Sets static CertStore that will handle TLS requests (find the appropriate cert for the connection)
  ///
  /// Ensure to sync NTP before using TLS (or it will report an invalid date for the certificates)
  ///
  /// UB if it Network outlives it
  static void setCertStore(driver::CertStore &store) noexcept;

  /// Sets new firmware update hook for this. Very useful to support upgrades
  /// reported by the network (LAST_VERSION header different than current
  /// sketch hash) Default is a noop
  static void setUpgradeHook(UpgradeHook scheduler) noexcept;
  /// Removes current hook, replaces for default one (noop)
  static auto takeUpgradeHook() noexcept -> UpgradeHook;

  /// Disconnects from WiFi
  static void disconnect() noexcept;
  /// Checks if WiFi connection is available (doesn't ensure WiFi actually has internet connection)
  static auto isConnected() noexcept -> bool;

  /// Sends an HTTP post that is authenticated to the monitor server.
  auto httpPost(std::string_view token, StaticString path, std::string_view data) const noexcept -> std::variant<Response, int>;

  /// Sends an HTTP post that is not authenticated to the monitor server (used for authentication).
  auto httpPost(StaticString path, std::string_view data) const noexcept -> std::variant<Response, int>;

  /// Sends an HTTP get that is authenticated to the monitor server (used for authentication).
  auto httpGet(StaticString path, std::string_view token, std::string_view data) const noexcept -> std::variant<Response, int>;

  /// Fetches firmware upgrade from the network
  auto upgrade(StaticString path, std::string_view token) const noexcept -> driver::UpgradeStatus;

  /// Extracts a network status from the raw response
  auto apiStatus(const driver::RawStatus &raw) const noexcept -> std::optional<NetworkStatus>;

  auto logger() const noexcept -> const Log &;
};

} // namespace iop
#endif