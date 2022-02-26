#include "driver/client.hpp"
#include "driver/panic.hpp"
#include "driver/network.hpp"
#include "sys/pgmspace.h"
#include "ESP8266HTTPClient.h"
#include "WiFiClientSecure.h"
#include <charconv>
#include <system_error>

namespace driver {

auto rawStatus(const int code) noexcept -> RawStatus {
  IOP_TRACE();
  switch (code) {
  case 200:
    return RawStatus::OK;
  case 500:
    return RawStatus::SERVER_ERROR;
  case 403:
    return RawStatus::FORBIDDEN;
  case HTTPC_ERROR_CONNECTION_FAILED:
    return RawStatus::CONNECTION_FAILED;
  case HTTPC_ERROR_SEND_HEADER_FAILED:
  case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
    return RawStatus::SEND_FAILED;
  case HTTPC_ERROR_NOT_CONNECTED:
  case HTTPC_ERROR_CONNECTION_LOST:
    return RawStatus::CONNECTION_LOST;
  case HTTPC_ERROR_NO_STREAM:
    return RawStatus::READ_FAILED;
  case HTTPC_ERROR_NO_HTTP_SERVER:
    return RawStatus::NO_SERVER;
  case HTTPC_ERROR_ENCODING:
    // Unsupported Transfer-Encoding header, if set it must be "chunked"
    return RawStatus::ENCODING_NOT_SUPPORTED;
  case HTTPC_ERROR_STREAM_WRITE:
    return RawStatus::READ_FAILED;
  case HTTPC_ERROR_READ_TIMEOUT:
    return RawStatus::READ_TIMEOUT;

  // We generally don't use default to be able to use static-analyzers to check
  // for exaustiveness, but this is a switch on a int, so...
  default:
    return RawStatus::UNKNOWN;
  }
}

Session::Session(HTTPClient &http, std::string_view uri) noexcept: http(std::ref(http)), uri_(uri) { IOP_TRACE(); }
Session::~Session() noexcept {
  IOP_TRACE();
  if (this->http && this->http->get().http)
    this->http->get().http->end();
}
void HTTPClient::headersToCollect(std::vector<std::string> headers) noexcept {
  iop_assert(this->http, IOP_STR("HTTP client is nullptr"));
  std::vector<const char*> normalized;
  normalized.reserve(headers.size());
  for (const auto &str: headers) {
    normalized.push_back(str.c_str());
  }
  this->http->collectHeaders(normalized.data(), headers.size());
}
std::string Response::header(iop::StaticString key) const noexcept {
  const auto value = this->headers_.find(key.toString());
  if (value == this->headers_.end()) return "";
  return std::move(value->second);
}
void Session::addHeader(iop::StaticString key, iop::StaticString value) noexcept {
  iop_assert(this->http && this->http->get().http, IOP_STR("Session has been moved out"));
  this->http->get().http->addHeader(String(key.get()), String(value.get()));
}
void Session::addHeader(iop::StaticString key, std::string_view value) noexcept {
  iop_assert(this->http && this->http->get().http, IOP_STR("Session has been moved out"));
  String val;
  val.concat(value.begin(), value.length());
  this->http->get().http->addHeader(String(key.get()), val);
}
void Session::addHeader(std::string_view key, iop::StaticString value) noexcept {
  iop_assert(this->http && this->http->get().http, IOP_STR("Session has been moved out"));
  String header;
  header.concat(key.begin(), key.length());
  this->http->get().http->addHeader(header, String(value.get()));
}
void Session::addHeader(std::string_view key, std::string_view value) noexcept {
  iop_assert(this->http && this->http->get().http, IOP_STR("Session has been moved out"));
  String val;
  val.concat(value.begin(), value.length());

  String header;
  header.concat(key.begin(), key.length());
  this->http->get().http->addHeader(header, val);
}
void Session::setAuthorization(std::string auth) noexcept {
  iop_assert(this->http && this->http->get().http, IOP_STR("Session has been moved out"));
  this->http->get().http->setAuthorization(auth.c_str());
}
auto Session::sendRequest(const std::string method, const std::string_view data) noexcept -> std::variant<Response, int> {
  iop_assert(this->http && this->http->get().http, IOP_STR("Session has been moved out"));
  const auto code = this->http->get().http->sendRequest(method.c_str(), reinterpret_cast<const uint8_t*>(data.begin()), data.length());
  if (code < 0) {
    return code;
  }

  std::unordered_map<std::string, std::string> headers;
  headers.reserve(this->http->get().http->headers());
  for (int index = 0; index < this->http->get().http->headers(); ++index) {
    const auto key = std::string(this->http->get().http->headerName(index).c_str());
    const auto value = std::string(this->http->get().http->header(index).c_str());
    headers[key] = value;
  }

  const auto httpString = this->http->get().http->getString();
  auto storage = std::vector<uint8_t>();
  storage.insert(storage.end(), httpString.c_str(), httpString.c_str() + httpString.length());
  const auto payload = Payload(storage);
  const auto response = Response(headers, payload, code);
  return response;
}

HTTPClient::HTTPClient() noexcept: http(new (std::nothrow) ::HTTPClient()) {
  iop_assert(http, IOP_STR("OOM"));
}
HTTPClient::~HTTPClient() noexcept {
  delete this->http;
}

std::optional<Session> HTTPClient::begin(std::string_view uri) noexcept {
  IOP_TRACE(); 
  
  //iop_assert(iop::wifi.client, IOP_STR("Wifi has been moved out, client is nullptr"));
  //iop::wifi.client.setNoDelay(false);
  //iop::wifi.client.setSync(true);

  // We can afford bigger timeouts since we shouldn't make frequent requests
  //constexpr uint16_t oneMinuteMs = 3 * 60 * 1000;
  //this->http.setTimeout(oneMinuteMs);

  //this->http.setAuthorization("");

  // Parse URI
  auto index = uri.find("://");
  if (index == uri.npos) {
    index = 0;
  } else {
    index += 3;
  }

  auto host = std::string_view(uri).substr(index);
  host = host.substr(0, host.find('/'));

  auto portStr = std::string_view();
  index = host.find(':');

  if (index != host.npos) {
    portStr = host.substr(index + 1);
    host = host.substr(0, index);
  } else if (uri.find("http://") != uri.npos) {
    portStr = "80";
  } else if (uri.find("https://") != uri.npos) {
    portStr = "443";
  } else {
    iop_panic(IOP_STR("Protocol missing inside HttpClient::begin"));
  }

  uint16_t port;
  auto result = std::from_chars(portStr.data(), portStr.data() + portStr.size(), port);
  if (result.ec != std::errc()) {
    iop_panic(IOP_STR("Unable to confert port to uint16_t: ").toString() + portStr.begin() + IOP_STR(" ").toString() + std::error_condition(result.ec).message());
  }

  iop_assert(iop::wifi.client, IOP_STR("Wifi has been moved out, client is nullptr"));
  iop_assert(this->http, IOP_STR("HTTP client is nullptr"));
  //this->http.setReuse(false);
  auto uriArduino = String();
  uriArduino.concat(uri.begin(), uri.length());
  if (this->http->begin(*iop::wifi.client, uriArduino)) {
    return Session(*this, uri);
  }

  return std::nullopt;
}
}