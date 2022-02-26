#include "driver/client.hpp"

namespace driver {
auto rawStatus(const int code) noexcept -> RawStatus {
  switch (code) {
  case 200:
    return RawStatus::OK;
  case 500:
    return RawStatus::SERVER_ERROR;
  case 403:
    return RawStatus::FORBIDDEN;

  // We generally don't use default to be able to use static-analyzers to check
  // for exaustiveness, but this is a switch on a int, so...
  default:
    return RawStatus::UNKNOWN;
  }
}

Session::~Session() noexcept {}
void HTTPClient::headersToCollect(std::vector<std::string> headers) noexcept { (void) headers; }
auto Response::header(iop::StaticString key) const noexcept -> std::optional<std::string> { (void) key; return std::nullopt; }
void Session::addHeader(iop::StaticString key, iop::StaticString value) noexcept { (void) key; (void) value; }
void Session::addHeader(iop::StaticString key, std::string_view value) noexcept { (void) key; (void) value; }
void Session::addHeader(std::string_view key, iop::StaticString value) noexcept { (void) key; (void) value; }
void Session::addHeader(std::string_view key, std::string_view value) noexcept { (void) key; (void) value; }
void Session::setAuthorization(std::string auth) noexcept { (void) auth; }
auto Session::sendRequest(const std::string method, const std::string_view data) noexcept -> std::variant<Response, int> { (void) method; (void) data; return 500; }

HTTPClient::HTTPClient() noexcept {}
HTTPClient::~HTTPClient() noexcept {}

auto HTTPClient::begin(std::string_view uri) noexcept -> std::optional<Session> { (void) uri; return std::nullopt; }
}