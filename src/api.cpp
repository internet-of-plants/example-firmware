#include "ESP8266httpUpdate.h"

#include "api.hpp"
#include "fixed_string.hpp"
#include "utils.hpp"

#ifndef IOP_API_DISABLED
auto Api::loggerLevel() const noexcept -> LogLevel {
  return this->logger.level();
}

/// Ok (200): success
/// Forbidden (403): auth token is invalid
/// Not Found (404): invalid plant id (if any) (is it owned by another account?)
/// Bad request (400): the json didn't fit the local buffer, this bad
/// No HttpCode and Internal Error (500): bad vibes at the wlan/server
auto Api::reportPanic(const AuthToken &authToken, const MacAddress &mac,
                      const PanicData &event) const noexcept -> ApiStatus {
  this->logger.debug(F("Report panic:"), event.msg);

  const auto make = [authToken, &mac, event](JsonDocument &doc) {
    doc["mac"] = mac.asString().get();
    doc["file"] = event.file.get();
    doc["line"] = event.line;
    doc["func"] = event.func.get();
    doc["msg"] = event.msg.get();
  };

  const auto maybeJson = this->makeJson<2048>(F("Api::reportPanic"), make);
  if (maybeJson.isNone())
    return ApiStatus::CLIENT_BUFFER_OVERFLOW;

  const auto token = authToken.asString();
  const auto &json = UNWRAP_REF(maybeJson);
  const auto maybeResp =
      this->network().httpPost(token, F("/device-panic"), json);

#ifndef IOP_MOCK_MONITOR
  if (IS_ERR(maybeResp)) {
    const auto code = std::to_string(UNWRAP_ERR_REF(maybeResp));
    this->logger.error(F("Unexpected response at Api::reportPanic: "), code);
    return ApiStatus::BROKEN_SERVER;
  }
  return UNWRAP_OK_REF(maybeResp).status;
#else
  return ApiStatus::OK;
#endif
}

/// Ok (200): success
/// Forbidden (403): auth token is invalid
/// Not Found (404): plant id is invalid (maybe it's owned by another account?)
/// Must Upgrade (412): event saved, but firmware is outdated, must upgrade it
/// Bad request (400): the json didn't fit the local buffer, this bad No
/// HttpCode and Internal Error (500): bad vibes at the wlan/server
auto Api::registerEvent(const AuthToken &authToken,
                        const Event &event) const noexcept -> ApiStatus {
  this->logger.debug(F("Send event: "), event.mac.asString());

  const auto make = [&event](JsonDocument &doc) {
    doc["air_temperature_celsius"] = event.storage.airTemperatureCelsius;
    doc["air_humidity_percentage"] = event.storage.airHumidityPercentage;
    doc["air_heat_index_celsius"] = event.storage.airHeatIndexCelsius;
    doc["soil_temperature_celsius"] = event.storage.soilTemperatureCelsius;
    doc["soil_resistivity_raw"] = event.storage.soilResistivityRaw;
    doc["firmware_hash"] = event.firmwareHash.asString().get();
    doc["mac"] = event.mac.asString().get();
  };
  const auto maybeJson = this->makeJson<256>(F("Api::registerEvent"), make);
  if (maybeJson.isNone())
    return ApiStatus::CLIENT_BUFFER_OVERFLOW;

  const auto token = authToken.asString();
  const auto &json = UNWRAP_REF(maybeJson);
  const auto maybeResp = this->network().httpPost(token, F("/event"), json);

#ifndef IOP_MOCK_MONITOR
  if (IS_ERR(maybeResp)) {
    const auto code = std::to_string(UNWRAP_ERR_REF(maybeResp));
    this->logger.error(F("Unexpected response at Api::registerEvent: "), code);
    return ApiStatus::BROKEN_SERVER;
  }
  return UNWRAP_OK_REF(maybeResp).status;
#else
  return ApiStatus::OK;
#endif
}

/// ApiStatus::NOT_FOUND: invalid credentials
/// ApiStatus::CLIENT_BUFFER_OVERFLOW: json didn't fit. This bad
/// ApiStatus::BROKEN_SERVER: Unexpected/broken response or too big
auto Api::authenticate(const StringView username, const StringView password,
                       const MacAddress &mac) const noexcept
    -> Result<AuthToken, ApiStatus> {
  this->logger.debug(F("Authenticate IoP user: "), username);

  if (username.isEmpty() || password.isEmpty()) {
    this->logger.debug(F("Empty username or password, at Api::authenticate"));
    return ApiStatus::FORBIDDEN;
  }

  const auto make = [username, password, &mac](JsonDocument &doc) {
    doc["email"] = username.get();
    doc["password"] = password.get();
    doc["mac"] = mac.asString().get();
  };
  const auto maybeJson = this->makeJson<256>(F("Api::authenticate"), make);
  if (maybeJson.isNone())
    return ApiStatus::CLIENT_BUFFER_OVERFLOW;

  const auto &json = UNWRAP_REF(maybeJson);
  auto maybeResp = this->network().httpPost(F("/user/login"), json);

#ifndef IOP_MOCK_MONITOR
  if (IS_ERR(maybeResp)) {
    const auto code = std::to_string(UNWRAP_ERR_REF(maybeResp));
    this->logger.error(F("Unexpected response at Api::authenticate: "), code);
    return ApiStatus::BROKEN_SERVER;
  }
  auto resp = UNWRAP_OK(maybeResp);

  if (resp.status != ApiStatus::OK && resp.status != ApiStatus::MUST_UPGRADE)
    return resp.status;

  if (resp.payload.isNone()) {
    this->logger.error(F("Server answered OK, but payload is missing"));
    return ApiStatus::BROKEN_SERVER;
  }

  const auto payload = UNWRAP(resp.payload);
  auto result = AuthToken::fromString(payload);

  if (IS_ERR(result)) {
    switch (UNWRAP_ERR(result)) {
    case TOO_BIG:
      const auto lengthStr = std::to_string(payload.length());
      this->logger.error(F("Auth token is too big: size = "), lengthStr);
      break;
    }

    return ApiStatus::BROKEN_SERVER;
  }

  return UNWRAP_OK(result);

#else
  return AuthToken::empty();
#endif
}

auto Api::registerLog(const AuthToken &authToken, const MacAddress &mac,
                      const StringView log) const noexcept -> ApiStatus {
  const auto token = authToken.asString();
  this->logger.debug(F("Register log. Token: "), token, F(". Log: "), log);
  // TODO(pc): dynamically generate the log string, instead of buffering it,
  // similar to a Stream but we choose when to write (using logger template
  // variadics)
  String uri(F("/log/"));
  uri += mac.asString().get();
  auto maybeResp = this->network().httpPost(token, uri, log);

#ifndef IOP_MOCK_MONITOR
  if (IS_ERR(maybeResp)) {
    const auto code = std::to_string(UNWRAP_ERR_REF(maybeResp));
    this->logger.error(F("Unexpected response at Api::registerLog: "), code);
    return ApiStatus::BROKEN_SERVER;
  }

  return UNWRAP_OK(maybeResp).status;
#else
  return ApiStatus::OK;
#endif
}

extern "C" uint32_t _FS_start;
extern "C" uint32_t _FS_end;

auto Api::upgrade(const AuthToken &token, const MacAddress &mac,
                  const MD5Hash &sketchHash) const noexcept -> ApiStatus {
  this->logger.debug(F("Upgrading sketch"));

  const auto tok = token.asString();

  String path(F("/update/"));
  path += tok.get();
  path += F("/");
  path += mac.asString().get();

  auto clientResult = this->network().wifiClient(path);
  if (IS_ERR(clientResult)) {
    const auto rawStatus = UNWRAP_ERR_REF(clientResult);
    const auto apiStatus = this->network().apiStatus(rawStatus);
    if (apiStatus.isSome())
      return UNWRAP_REF(apiStatus);

    const auto s = Network::rawStatusToString(rawStatus);
    this->logger.warn(F("Api::upgrade returned invalid RawStatus: "), s);
    return ApiStatus::BROKEN_SERVER;
  }
  auto &client = UNWRAP_OK_MUT(clientResult).get();

  const auto *const version = sketchHash.asString().get();
  // TODO: upstream we already can use the setAuthorization header, but it's not
  // published
  const auto uri = String(this->host().asCharPtr()) + path;

  ESPhttpUpdate.closeConnectionsOnUpdate(true);
  ESPhttpUpdate.rebootOnUpdate(true);
  ESPhttpUpdate.setLedPin(LED_BUILTIN);
  const auto updateResult = ESPhttpUpdate.updateFS(client, uri, version);

#ifdef IOP_MOCK_MONITOR
  return ApiStatus::OK;
#endif

  switch (updateResult) {
  case HTTP_UPDATE_NO_UPDATES:
    return ApiStatus::NOT_FOUND;
  case HTTP_UPDATE_OK:
    return ApiStatus::OK;
  case HTTP_UPDATE_FAILED:
  default:
    // TODO(pc): properly handle ESPhttpUpdate.getLastError()
    this->logger.error(F("Update failed: "),
                       ESPhttpUpdate.getLastErrorString());
    return ApiStatus::BROKEN_SERVER;
  }
}
#endif

#ifdef IOP_API_DISABLED
void Api::setup() const { Network::setup(); }

bool Api::isConnected() const noexcept { return true; }
void Api::disconnect() const noexcept {}
LogLevel Api::loggerLevel() const noexcept { return this->logger.level(); }

ApiStatus Api::upgrade(const AuthToken &token,
                       const MD5Hash sketchHash) const noexcept {
  (void)token;
  (void)sketchHash;
  return Option<HttpCode>(200);
}
ApiStatus Api::registerEvent(const AuthToken &authToken,
                             const Event &event) const noexcept {
  return Option<HttpCode>(200);
}

Result<AuthToken, ApiStatus>
Api::authenticate(const StringView username,
                  const StringView password) const noexcept {
  return AuthToken::empty();
}
#endif