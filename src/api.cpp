#include "api.hpp"
#include "driver/client.hpp"
#include "driver/server.hpp"
#include "driver/panic.hpp"
#include "generated/certificates.hpp"
#include "utils.hpp"
#include "loop.hpp"
#include <string>

// TODO: have an endpoint to report non 200 response
// TODO: have an endpoint to report BROKEN_CLIENTS

#ifndef IOP_API_DISABLED

// If IOP_MONITOR is not defined the Api methods will be short-circuited
// If IOP_MOCK_MONITOR is defined, then the methods will run normally and pretend the request didn't fail
// If IOP_MONITOR is defined, then IOP_MOCK_MONITOR shouldn't be defined
#ifdef IOP_MONITOR
#undef IOP_MOCK_MONITOR
#endif

using FixedJsonBuffer = StaticJsonDocument<Api::JsonCapacity>;

auto Api::makeJson(const iop::StaticString contextName, const JsonCallback &jsonObjectBuilder) const noexcept -> std::unique_ptr<Api::Json> {
  IOP_TRACE();
  
  auto doc = std::unique_ptr<FixedJsonBuffer>(new (std::nothrow) FixedJsonBuffer());
  if (!doc) return nullptr;
  doc->clear(); // Zeroes heap memory
  jsonObjectBuilder(*doc);

  if (doc->overflowed()) {
    this->logger.error(IOP_STR("Payload doesn't fit buffer at "), contextName);
    return nullptr;
  }

  auto json = std::unique_ptr<Api::Json>(new (std::nothrow) Api::Json());
  if (!json) return nullptr;
  json->fill('\0'); // Zeroes heap memory
  serializeJson(*doc, json->data(), json->size());
  
  // This might leak sensitive information
  this->logger.debug(IOP_STR("Json: "), iop::to_view(*json));
  return json;
}

#ifdef IOP_ONLINE
static void upgradeScheduler() noexcept {
  utils::scheduleInterrupt(InterruptEvent::MUST_UPGRADE);
}
void onWifiConnect() noexcept {
  utils::scheduleInterrupt(InterruptEvent::ON_CONNECTION);
}
#endif

auto Api::setup() const noexcept -> void {
  IOP_TRACE();
#ifdef IOP_ONLINE

  iop::wifi.onConnect(onWifiConnect);
  // If we are already connected the callback won't be called
  if (iop::Network::isConnected())
    utils::scheduleInterrupt(InterruptEvent::ON_CONNECTION);

  // Sets scheduler for upgrade interrupt
  iop::Network::setUpgradeHook(iop::UpgradeHook(upgradeScheduler));

#ifdef IOP_SSL
  static driver::CertStore certStore(generated::certList);
  this->network.setCertStore(certStore);
#endif

  this->network.setup();
#endif
}

auto Api::reportPanic(const AuthToken &authToken, const PanicData &event) const noexcept -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.info(IOP_STR("Report iop_panic: "), event.msg);

  auto msg = event.msg;
  auto json = std::unique_ptr<Api::Json>();

  while (true) {
    const auto make = [event, &msg](JsonDocument &doc) {
      doc["file"] = event.file.toString();
      doc["line"] = event.line;
      doc["func"] = event.func.toString();
      doc["msg"] = msg;
    };
    json = this->makeJson(IOP_STR("Api::reportPanic"), make);

    if (!json) {
      iop_assert(msg.length() / 2 != 0, IOP_STR("Message would be empty, function is broken"));
      msg = msg.substr(0, msg.length() / 2);
      continue;
    }
    break;
  }

  if (!json)
    return iop::NetworkStatus::BROKEN_CLIENT;

  const auto token = iop::to_view(authToken);
  const auto status = this->network.httpPost(token, IOP_STR("/v1/panic"), iop::to_view(*json));

#ifndef IOP_MOCK_MONITOR
  if (const auto *error = std::get_if<int>(&status)) {
    const auto code = std::to_string(*error);
    this->logger.error(IOP_STR("Unexpected response at Api::reportPanic: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  } else if (const auto *response = std::get_if<iop::Response>(&status)) {
    return response->status();
  }
  iop_panic(IOP_STR("Invalid variant"));
#else
  return iop::NetworkStatus::OK;
#endif
}

auto Api::registerEvent(const AuthToken &authToken, const Event &event) const noexcept -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.info(IOP_STR("Send event"));

  const auto make = [&event](JsonDocument &doc) {
    doc["air_temperature_celsius"] = event.airTemperatureCelsius;
    doc["air_humidity_percentage"] = event.airHumidityPercentage;
    doc["air_heat_index_celsius"] = event.airHeatIndexCelsius;
    doc["soil_temperature_celsius"] = event.soilTemperatureCelsius;
    doc["soil_resistivity_raw"] = event.soilResistivityRaw;
  };

  const auto json = this->makeJson(IOP_STR("Api::registerEvent"), make);
  if (!json)
    return iop::NetworkStatus::BROKEN_CLIENT;

  const auto token = iop::to_view(authToken);
  const auto status = this->network.httpPost(token, IOP_STR("/v1/event"), iop::to_view(*json));

#ifndef IOP_MOCK_MONITOR
  if (const auto *error = std::get_if<int>(&status)) {
    const auto code = std::to_string(*error);
    this->logger.error(IOP_STR("Unexpected response at Api::registerEvent: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  } else if (const auto *response = std::get_if<iop::Response>(&status)) {
    return response->status();
  }
  iop_panic(IOP_STR("Invalid variant"));
#else
  return iop::NetworkStatus::OK;
#endif
}
auto Api::authenticate(std::string_view username, std::string_view password) const noexcept -> std::variant<AuthToken, iop::NetworkStatus> {
  IOP_TRACE();
  this->logger.info(IOP_STR("Authenticate IoP user: "), username);

  if (!username.length() || !password.length()) {
    this->logger.warn(IOP_STR("Empty username or password, at Api::authenticate"));
    return iop::NetworkStatus::FORBIDDEN;
  }

  const auto make = [username, password](JsonDocument &doc) {
    doc["email"] = username;
    doc["password"] = password;
  };
  const auto json = this->makeJson(IOP_STR("Api::authenticate"), make);

  if (!json)
    return iop::NetworkStatus::BROKEN_CLIENT;
  const auto status = this->network.httpPost(IOP_STR("/v1/user/login"), iop::to_view(*json));

#ifndef IOP_MOCK_MONITOR
  if (const auto *error = std::get_if<int>(&status)) {
    const auto code = std::to_string(*error);
    this->logger.error(IOP_STR("Unexpected response at Api::authenticate: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  } else if (const auto *response = std::get_if<iop::Response>(&status)) {
    if (response->status() != iop::NetworkStatus::OK) {
      return response->status();
    }

    const auto payload = response->payload();
    if (!payload) {
      this->logger.error(IOP_STR("Server answered OK, but payload is missing"));
      return iop::NetworkStatus::BROKEN_SERVER;
    }

    if (!iop::isAllPrintable(*payload)) {
      this->logger.error(IOP_STR("Unprintable payload, this isn't supported: "), iop::to_view(iop::scapeNonPrintable(*payload)));
      return iop::NetworkStatus::BROKEN_SERVER;
    }
    if (payload->length() != 64) {
      this->logger.error(IOP_STR("Auth token does not occupy 64 bytes: size = "), std::to_string(payload->length()));
    }

    AuthToken token;
    memcpy(token.data(), &payload->front(), payload->length());
    return token;
  }
  iop_panic(IOP_STR("Invalid variant"));
#else
  return AuthToken::empty();
#endif
}

auto Api::registerLog(const AuthToken &authToken, std::string_view log) const noexcept -> iop::NetworkStatus {
  IOP_TRACE();
  const auto token = iop::to_view(authToken);
  this->logger.info(IOP_STR("Register log. Token: "), token, IOP_STR(". Log: "), log);
  auto const status = this->network.httpPost(token, IOP_STR("/v1/log"), log);

#ifndef IOP_MOCK_MONITOR
  if (const auto *error = std::get_if<int>(&status)) {
    const auto code = std::to_string(*error);
    this->logger.error(IOP_STR("Unexpected response at Api::registerLog: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  } else if (const auto *response = std::get_if<iop::Response>(&status)) {
    return response->status();
  }
  iop_panic(IOP_STR("Invalid variant"));
#else
  return iop::NetworkStatus::OK;
#endif
}

auto Api::upgrade(const AuthToken &token) const noexcept
    -> driver::UpgradeStatus {
  IOP_TRACE();
  this->logger.info(IOP_STR("Upgrading sketch"));

  return this->network.upgrade(IOP_STR("/v1/update"), iop::to_view(token));
}
#else
auto Api::upgrade(const AuthToken &token) const noexcept
    -> driver::UpgradeStatus {
  (void)*this;
  (void)token;
  IOP_TRACE();
  return driver::UpgradeStatus::NO_UPGRADE;
}
auto Api::reportPanic(const AuthToken &authToken,
                      const PanicData &event) const noexcept
    -> iop::NetworkStatus {
  (void)*this;
  (void)authToken;
  (void)event;
  IOP_TRACE();
  return iop::NetworkStatus::OK;
}
auto Api::registerEvent(const AuthToken &token,
                        const Event &event) const noexcept
    -> iop::NetworkStatus {
  (void)*this;
  (void)token;
  (void)event;
  IOP_TRACE();
  return iop::NetworkStatus::OK;
}
auto Api::authenticate(std::string_view username,
                       std::string_view password) const noexcept
    -> std::variant<AuthToken, iop::NetworkStatus> {
  (void)*this;
  (void)std::move(username);
  (void)std::move(password);
  IOP_TRACE();
  return (AuthToken){0};
}
auto Api::registerLog(const AuthToken &authToken,
                      std::string_view log) const noexcept
    -> iop::NetworkStatus {
  (void)*this;
  (void)authToken;
  (void)std::move(log);
  IOP_TRACE();
  return iop::NetworkStatus::OK;
}

auto Api::setup() const noexcept -> void {}
#endif

Api::Api(iop::StaticString uri, const iop::LogLevel logLevel) noexcept
    : network(std::move(uri), logLevel), logger(logLevel, IOP_STR("API")) {
  IOP_TRACE();
}