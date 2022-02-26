#include "driver/network.hpp"
#include "driver/cert_store.hpp"
#include "driver/thread.hpp"
#include "driver/upgrade.hpp"

constexpr static iop::UpgradeHook defaultHook(iop::UpgradeHook::defaultHook);

static iop::UpgradeHook hook(defaultHook);
static driver::CertStore * maybeCertStore = nullptr;

namespace iop {
driver::Wifi wifi;

auto Network::logger() const noexcept -> const Log & {
  return this->logger_;
}
auto Network::upgrade(StaticString path, std::string_view token) const noexcept -> driver::UpgradeStatus {
  return driver::Upgrade::run(*this, path, token);
}
void UpgradeHook::defaultHook() noexcept { IOP_TRACE(); }
void Network::setCertStore(driver::CertStore &store) noexcept {
  maybeCertStore = &store;
}
void Network::setUpgradeHook(UpgradeHook scheduler) noexcept {
  hook = std::move(scheduler);
}
auto Network::takeUpgradeHook() noexcept -> UpgradeHook {
  auto old = hook;
  hook = defaultHook;
  return old;
}

auto Network::isConnected() noexcept -> bool {
  return iop::wifi.status() == driver::StationStatus::GOT_IP;
}

void Network::disconnect() noexcept {
  IOP_TRACE();
  return iop::wifi.disconnectFromAccessPoint();
}

auto Network::httpPost(std::string_view token, const StaticString path, std::string_view data) const noexcept -> std::variant<Response, int> {
  return this->httpRequest(HttpMethod::POST, token, path, data);
}

auto Network::httpPost(StaticString path, std::string_view data) const noexcept -> std::variant<Response, int> {
  return this->httpRequest(HttpMethod::POST, std::nullopt, path, data);
}

auto Network::httpGet(StaticString path, std::string_view token, std::string_view data) const noexcept -> std::variant<Response, int> {
  return this->httpRequest(HttpMethod::GET, token, path, data);
}

auto Network::apiStatus(const driver::RawStatus &raw) const noexcept -> std::optional<NetworkStatus> {
  IOP_TRACE();
  switch (raw) {
  case driver::RawStatus::CONNECTION_FAILED:
  case driver::RawStatus::CONNECTION_LOST:
    this->logger().warn(IOP_STR("Connection failed. Code: "), std::to_string(static_cast<int>(raw)));
    return NetworkStatus::IO_ERROR;

  case driver::RawStatus::SEND_FAILED:
  case driver::RawStatus::READ_FAILED:
    this->logger().warn(IOP_STR("Pipe is broken. Code: "), std::to_string(static_cast<int>(raw)));
    return NetworkStatus::IO_ERROR;

  case driver::RawStatus::ENCODING_NOT_SUPPORTED:
  case driver::RawStatus::NO_SERVER:
  case driver::RawStatus::SERVER_ERROR:
    this->logger().error(IOP_STR("Server is broken. Code: "), std::to_string(static_cast<int>(raw)));
    return NetworkStatus::BROKEN_SERVER;

  case driver::RawStatus::READ_TIMEOUT:
    this->logger().warn(IOP_STR("Network timeout triggered"));
    return NetworkStatus::IO_ERROR;

  case driver::RawStatus::OK:
    return NetworkStatus::OK;

  case driver::RawStatus::FORBIDDEN:
    return NetworkStatus::FORBIDDEN;

  case driver::RawStatus::UNKNOWN:
    break;
  }
  return std::nullopt;
}

Network::Network(StaticString uri, const LogLevel &logLevel) noexcept
  : logger_(logLevel, IOP_STR("NETWORK")), uri_(std::move(uri)) {
  IOP_TRACE();
}

Response::Response(const NetworkStatus &status) noexcept
    : status_(status), payload_(std::optional<std::vector<uint8_t>>()) {
  IOP_TRACE();
}
Response::Response(const NetworkStatus &status, std::vector<uint8_t> payload) noexcept
    : status_(status), payload_(payload) {
  IOP_TRACE();
}
auto Response::status() const noexcept -> NetworkStatus {
  return this->status_;
}
auto Response::payload() const noexcept -> std::optional<std::string_view> {
  if (!this->payload_) return std::nullopt;
  return iop::to_view(*this->payload_);
}
}

#if !defined(IOP_ONLINE) || defined(IOP_NOOP)
#include "driver/noop/network.hpp"
#else

#include "driver/wifi.hpp"
#include "driver/device.hpp"
#include "driver/client.hpp"
#include "driver/panic.hpp"
#include "string.h"

static driver::HTTPClient http;

namespace iop {
static auto methodToString(const HttpMethod &method) noexcept -> StaticString;

// Returns Response if it can understand what the server sent, int is the raw
// response given by ESP8266HTTPClient
auto Network::httpRequest(const HttpMethod method_,
                          const std::optional<std::string_view> &token, StaticString path,
                          const std::optional<std::string_view> &data) const noexcept
    -> std::variant<Response, int> {
  IOP_TRACE();
  Network::setup();

  const auto uri = this->uri().toString() + path.toString();
  const auto method = methodToString(method_);

  const auto data_ = data.value_or(std::string_view());

  this->logger().debug(method, IOP_STR(" to "), this->uri(), path, IOP_STR(", data length: "), std::to_string(data_.length()));

  // TODO: this may log sensitive information, network logging is currently
  // capped at debug because of that
  if (data)
    this->logger().debug(*data);
  
  this->logger().debug(IOP_STR("Begin"));
  auto maybeSession = http.begin(uri);
  if (!maybeSession) {
    this->logger().warn(IOP_STR("Failed to begin http connection to "), iop::to_view(uri));
    return Response(NetworkStatus::IO_ERROR);
  }
  auto &session = *maybeSession;
  this->logger().trace(IOP_STR("Began HTTP connection"));

  if (token) {
    const auto tok = *token;
    session.setAuthorization(std::string(tok).c_str());
  }

  // Currently only JSON is supported
  if (data)
    session.addHeader(IOP_STR("Content-Type"), IOP_STR("application/json"));

  // Authentication headers, identifies device and detects updates, perf
  // monitoring
  {
    auto str = iop::to_view(driver::device.firmwareMD5());
    session.addHeader(IOP_STR("VERSION"), str);
    session.addHeader(IOP_STR("x-ESP8266-sketch-md5"), str);

    str = iop::to_view(driver::device.macAddress());
    session.addHeader(IOP_STR("MAC_ADDRESS"), str);
  }
   
  {
    // Could this cause memory fragmentation?
    auto memory = driver::device.availableMemory();
    
    session.addHeader(IOP_STR("FREE_STACK"), std::to_string(memory.availableStack));

    for (const auto & item: memory.availableHeap) {
      session.addHeader(IOP_STR("FREE_").toString().append(item.first), std::to_string(item.second));
    }
    for (const auto & item: memory.biggestHeapBlock) {
      session.addHeader(IOP_STR("BIGGEST_BLOCK_").toString().append(item.first), std::to_string(item.second));
    }
  }
  session.addHeader(IOP_STR("VCC"), std::to_string(driver::device.vcc()));
  session.addHeader(IOP_STR("TIME_RUNNING"), std::to_string(driver::thisThread.timeRunning()));
  session.addHeader(IOP_STR("ORIGIN"), this->uri());
  session.addHeader(IOP_STR("DRIVER"), driver::device.platform());

  this->logger().debug(IOP_STR("Making HTTP request"));

  auto responseVariant = session.sendRequest(method.toString(), data_);
  if (const auto *error = std::get_if<int>(&responseVariant)) {
    return *error;
  } else if (auto *response = std::get_if<driver::Response>(&responseVariant)) {
    this->logger().debug(IOP_STR("Made HTTP request")); 

    // Handle system upgrade request
    const auto upgrade = response->header(IOP_STR("LATEST_VERSION"));
    if (upgrade.length() > 0 && memcmp(upgrade.c_str(), driver::device.firmwareMD5().data(), 32) != 0) {
      this->logger().info(IOP_STR("Scheduled upgrade"));
      hook.schedule();
    }

    // TODO: move this to inside driver::HTTPClient logic
    const auto rawStatus = driver::rawStatus(response->status());
    if (rawStatus == driver::RawStatus::UNKNOWN) {
      this->logger().warn(IOP_STR("Unknown response code: "), std::to_string(response->status()));
    }
    
    const auto rawStatusStr = driver::rawStatusToString(rawStatus);

    this->logger().debug(IOP_STR("Response code ("), iop::to_view(std::to_string(response->status())), IOP_STR("): "), rawStatusStr);

    // TODO: this is broken because it's not lazy, it should be a HTTPClient setting that bails out if it's bigger, and encapsulated in the API
    /*
    constexpr const int32_t maxPayloadSizeAcceptable = 2048;
    if (globalData.http().getSize() > maxPayloadSizeAcceptable) {
      globalData.http().end();
      const auto lengthStr = std::to_string(response.payload.length());
      this->logger().error(IOP_STR("Payload from server was too big: "), lengthStr);
      globalData.response() = Response(NetworkStatus::BROKEN_SERVER);
      return globalData.response();
    }
    */

    // We have to simplify the errors reported by this API (but they are logged)
    const auto maybeApiStatus = this->apiStatus(rawStatus);
    if (maybeApiStatus) {
      // The payload is always downloaded, since we check for its size and the
      // origin is trusted. If it's there it's supposed to be there.
      auto payload = std::move(response->await().payload);
      this->logger().debug(IOP_STR("Payload (") , std::to_string(payload.size()), IOP_STR("): "), iop::to_view(iop::scapeNonPrintable(iop::to_view(payload).substr(0, payload.size() > 30 ? 30 : payload.size()))));
      return Response(*maybeApiStatus, std::move(payload));
    }
    return response->status();
  }
  iop_panic(IOP_STR("Invalid variant types"));
}

void Network::setup() const noexcept {
  IOP_TRACE();
  static bool initialized = false;
  if (initialized) return;
  initialized = true;

  std::vector<std::string> headers = {"LATEST_VERSION"};
  http.headersToCollect(headers);

  iop::wifi.setup(maybeCertStore);
}

static auto methodToString(const HttpMethod &method) noexcept -> StaticString {
  IOP_TRACE();
  switch (method) {
  case HttpMethod::GET:
    return IOP_STR("GET");

  case HttpMethod::HEAD:
    return IOP_STR("HEAD");

  case HttpMethod::POST:
    return IOP_STR("POST");

  case HttpMethod::PUT:
    return IOP_STR("PUT");

  case HttpMethod::PATCH:
    return IOP_STR("PATCH");

  case HttpMethod::DELETE:
    return IOP_STR("DELETE");

  case HttpMethod::CONNECT:
    return IOP_STR("CONNECT");
    
  case HttpMethod::OPTIONS:
    return IOP_STR("OPTIONS");
  }
  iop_panic(IOP_STR("HTTP Method not found"));
}
} // namespace iop
#endif