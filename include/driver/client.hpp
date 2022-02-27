#ifndef IOP_DRIVER_CLIENT_HPP
#define IOP_DRIVER_CLIENT_HPP

#include "driver/string.hpp"
#include "driver/response.hpp"
#include <functional>
#include <string>
#include <optional>
#include <memory>

class HTTPClient;

namespace driver {
class HTTPClient;

// References HTTPClient, should never outlive it
class Session {
  #ifdef IOP_POSIX
  int fd_;
  #endif

  #ifndef IOP_NOOP
  std::optional<std::reference_wrapper<HTTPClient>> http;
  std::unordered_map<std::string, std::string> headers;
  std::string_view uri_;
  #endif

  #ifdef IOP_POSIX
  Session(HTTPClient &http, std::string_view uri, int fd) noexcept;
  #elif defined(IOP_ESP8266)
  Session(HTTPClient &http, std::string_view uri) noexcept;
  #elif defined(IOP_NOOP)
  Session() noexcept = default;
  #else
  #error "Target is not valid"
  #endif

  friend HTTPClient;

public:
  void addHeader(iop::StaticString key, iop::StaticString value) noexcept;
  void addHeader(iop::StaticString key, std::string_view value) noexcept;
  void addHeader(std::string_view key, iop::StaticString value) noexcept;
  void addHeader(std::string_view key, std::string_view value) noexcept;
  void setAuthorization(std::string auth) noexcept;
  // How to represent that this moves the server out
  auto sendRequest(std::string method, std::string_view data) noexcept -> Response;
  Session(Session &&other) noexcept = default;
  Session(const Session &other) noexcept = delete;
  auto operator==(Session &&other) noexcept -> Session &;
  auto operator==(const Session &other) noexcept -> Session & = delete;
  ~Session() noexcept = default;
};

class HTTPClient {
#ifdef IOP_POSIX
  std::vector<std::string> headersToCollect_;
#elif defined(IOP_ESP8266)
  ::HTTPClient * http;
#elif defined(IOP_NOOP)
#else
#error "Target not valid"
#endif
public:
  HTTPClient() noexcept;
  ~HTTPClient() noexcept;
  auto begin(std::string_view uri, std::function<Response(Session &)> func) noexcept -> Response;
  auto headersToCollect(std::vector<std::string> headers) noexcept -> void;

  HTTPClient(HTTPClient &&other) noexcept;
  HTTPClient(const HTTPClient &other) noexcept = delete;
  auto operator==(HTTPClient &&other) noexcept -> HTTPClient &;
  auto operator==(const HTTPClient &other) noexcept -> HTTPClient & = delete;
  friend Session;
};
}

#ifdef IOP_POSIX
#ifdef IOP_SSL
#error "SSL not supported for desktop right now"
#endif
#endif

#endif