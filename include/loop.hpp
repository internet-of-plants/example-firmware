#ifndef IOP_LOOP
#define IOP_LOOP

#ifndef IOP_DESKTOP
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#define HIGH 0x1
#include "ESP8266httpUpdate.h"
#undef OUTPUT
#undef INPUT
#undef HIGH
#undef LOW
#undef RISING
#undef FALLING
#undef CHANGED
#undef LED_BUILTIN
#endif

#include "configuration.hpp"
#include "flash.hpp"
#include "sensors.hpp"
#include "server.hpp"
#include "api.hpp"
#include "utils.hpp"

#include <optional>

#include "driver/device.hpp"
#include "driver/thread.hpp"
#include "core/data.hpp"

class EventLoop {
private:
  CredentialsServer credentialsServer;
  Api api_;
  iop::Log logger;
  Flash flash_;
  Sensors sensors;

  iop::esp_time nextMeasurement;
  iop::esp_time nextYieldLog;
  iop::esp_time nextHandleConnectionLost;

  iop::esp_time nextTryFlashWifiCredentials = 0;
  iop::esp_time nextTryHardcodedWifiCredentials = 0;
  iop::esp_time nextTryHardcodedIopCredentials = 0;

public:
  Api const & api() const noexcept { return this->api_; }
  Flash const & flash() const noexcept { return this->flash_; }
  void setup() noexcept;
  void loop() noexcept;
  /// Connects to WiFi
  void connect(std::string_view ssid, std::string_view password) const noexcept;
  
  /// Uses IoP credentials to generate an authentication token for the device
  auto authenticate(std::string_view username,
                                     std::string_view password,
                                     const Api &api) const noexcept
    -> std::optional<std::array<char, 64>>;
  auto statusToString(const driver::StationStatus status)
    const noexcept -> std::optional<iop::StaticString>;

private:
  void handleInterrupt(const InterruptEvent event, const std::optional<AuthToken> &maybeToken) const noexcept;
  void handleCredentials() noexcept;
  void handleMeasurements(const AuthToken &token) noexcept;

public:
  explicit EventLoop(iop::StaticString uri, iop::LogLevel logLevel_) noexcept
      : credentialsServer(logLevel_),
        api_(std::move(uri), logLevel_),
        logger(logLevel_, F("LOOP")), flash_(logLevel_),
        sensors(config::soilResistivityPower, config::soilTemperature, config::airTempAndHumidity, config::dhtVersion),
        nextMeasurement(0), nextYieldLog(0), nextHandleConnectionLost(0),
        nextTryFlashWifiCredentials(0), nextTryHardcodedWifiCredentials(0), nextTryHardcodedIopCredentials(0) {
    IOP_TRACE();
  }
  ~EventLoop() noexcept = default;
  auto operator=(EventLoop const &other) noexcept -> EventLoop & = default;
  auto operator=(EventLoop &&other) noexcept -> EventLoop & = default;
  EventLoop(EventLoop const &other) noexcept = default;
  EventLoop(EventLoop &&other) noexcept = default;
};

extern EventLoop eventLoop;

class Unused4KbSysStack {
  struct StackStruct {
    std::array<char, 1024> text;
    #ifndef IOP_DESKTOP
    std::optional<ESP8266HTTPUpdate> updater;
    std::optional<ESP8266WebServer> server;
    std::optional<DNSServer> dns;
    #endif
    std::array<char, 64> token;
    std::array<char, 64> psk;
    std::array<char, 32> ssid;
  } *data;
  
  static_assert(sizeof(StackStruct) <= 4096);

public:
#ifdef IOP_DESKTOP
  Unused4KbSysStack() noexcept: data(new StackStruct) {
    memset((void*)data, 0, sizeof(StackStruct));
  }
  void reset() noexcept {
    memset((void*)data, 0, sizeof(StackStruct));
  }
#else
  Unused4KbSysStack() noexcept: data(new StackStruct) {}
  void reset() noexcept {}
  //Unused4KbSysStack() noexcept: data(reinterpret_cast<StackStruct *>(0x3FFFE000)) {
  //  memset((void*)0x3FFFE000, 0, 4096);
  //}
  //void reset() noexcept {
  //  memset((void*)0x3FFFE000, 0, 4096);
  //}
#endif
  #ifndef IOP_DESKTOP
  auto dns() noexcept -> DNSServer & {
    if (!this->data->dns.has_value())
      this->data->dns = std::make_optional(DNSServer());
    return iop::unwrap_mut(this->data->dns, IOP_CTX());
  }
  auto server() noexcept -> std::optional<ESP8266WebServer> & {
    return this->data->server;
  }
  auto updater() noexcept -> ESP8266HTTPUpdate & {
    if (!this->data->updater.has_value())
      this->data->updater = std::make_optional(ESP8266HTTPUpdate());
    return iop::unwrap_mut(this->data->updater, IOP_CTX());
  }
  #endif
  auto psk() noexcept -> std::array<char, 64> & {
    return this->data->psk;
  }
  auto ssid() noexcept -> std::array<char, 32> & {
    return this->data->ssid;
  }
  auto token() noexcept -> std::array<char, 64> & {
    return this->data->token;
  }
  auto text() noexcept -> std::array<char, 1024> & {
    return this->data->text;
  }
  // ...
};
extern Unused4KbSysStack unused4KbSysStack;

#endif