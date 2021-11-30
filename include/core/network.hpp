#ifndef IOP_NETWORK_HPP
#define IOP_NETWORK_HPP

#include <variant>
#include <optional>
#include <string>
#include "driver/client.hpp"
#include "core/log.hpp"

namespace iop {
/// Higher level error reporting. Lower level is logged
enum class NetworkStatus {
  CONNECTION_ISSUES,
  BROKEN_SERVER,
  CLIENT_BUFFER_OVERFLOW, // BROKEN_CLIENT

  OK,
  FORBIDDEN,
};

class Response;
enum class RawStatus;
enum class HttpMethod;
class CertStore;

/// If server set LAST_VERSION HTTP header is different than current sketch's
/// md5 hash the hook is called
class UpgradeHook {
public:
  using UpgradeScheduler = void (*) ();
  UpgradeScheduler schedule;

  constexpr explicit UpgradeHook(UpgradeScheduler scheduler) noexcept : schedule(scheduler) {}
  static void defaultHook() noexcept; // Noop
};

/// General lower level client network API, that is focused on our need.
/// Security, good error reporting, no UB possible, ergonomy.
///
/// Higher level than simple ESP8266 client network abstractions, but still
/// about http protocol
///
/// MUST CALL `setCertStore` otherwise TLS won't work. It _will_ panic.
///
/// `setUpgradeHook` to monitor when the server offers upgrade
///
/// Check `api.hpp` for even higher level code
class Network {
  Log logger;
  StaticString uri_;

public:
  Network(StaticString uri, const LogLevel &logLevel) noexcept;
  auto setup() const noexcept -> void;
  auto uri() const noexcept -> StaticString { return this->uri_; };

  static void setCertStore(CertStore &store) noexcept;

  /// Replaces current hook for this. Very useful to support upgrades
  /// reported by the network (LAST_VERSION header different than current
  /// sketch hash) Default is a noop
  static void setUpgradeHook(UpgradeHook scheduler) noexcept;
  /// Removes current hook, replaces for default one (noop)
  static auto takeUpgradeHook() noexcept -> UpgradeHook;

  static void disconnect() noexcept;
  static auto isConnected() noexcept -> bool;

  auto httpPost(std::string_view token, StaticString path,
                std::string_view data) const noexcept
      -> std::variant<Response, int>;
  auto httpPost(StaticString path, std::string_view data) const noexcept
      -> std::variant<Response, int>;

  auto httpRequest(HttpMethod method, const std::optional<std::string_view> &token,
                   StaticString path,
                   const std::optional<std::string_view> &data) const noexcept
      -> std::variant<Response, int>;

  static auto apiStatusToString(const NetworkStatus &status) noexcept
      -> StaticString;
  auto apiStatus(const driver::RawStatus &raw) const noexcept
      -> std::optional<NetworkStatus>;

  ~Network() noexcept;
  Network(Network const &other);
  Network(Network &&other) = delete;
  auto operator=(Network const &other) -> Network &;
  auto operator=(Network &&other) -> Network & = delete;
};

class Response {
public:
  NetworkStatus status;
  std::optional<std::string> payload;
  ~Response() noexcept;
  explicit Response(const NetworkStatus &status) noexcept;
  Response(const NetworkStatus &status, std::string payload) noexcept;
  Response(Response &resp) noexcept = delete;
  Response(Response &&resp) noexcept;
  auto operator=(Response &resp) noexcept -> Response & = delete;
  auto operator=(Response &&resp) noexcept -> Response &;
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
} // namespace iop
#endif