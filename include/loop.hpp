#ifndef IOP_LOOP
#define IOP_LOOP
#include "api.hpp"

#include "driver/device.hpp"
#include "driver/thread.hpp"
#include "driver/network.hpp"
#include "driver/panic.hpp"
#include "configuration.hpp"
#include "storage.hpp"
#include "sensors.hpp"
#include "server.hpp"
#include "utils.hpp"

#include <optional>

class EventLoop {
private:
  CredentialsServer credentialsServer;
  Api api_;
  iop::Log logger;
  Storage storage_;
  Sensors sensors;

  iop::time nextMeasurement;
  iop::time nextYieldLog;
  iop::time nextNTPSync;
  iop::time nextHandleConnectionLost;

  iop::time nextTryStorageWifiCredentials;
  iop::time nextTryHardcodedWifiCredentials;
  iop::time nextTryHardcodedIopCredentials;

public:
  Api const & api() const noexcept { return this->api_; }
  Storage const & storage() const noexcept { return this->storage_; }
  void setup() noexcept;
  void loop() noexcept;
  /// Connects to WiFi
  void connect(std::string_view ssid, std::string_view password) const noexcept;
  
  /// Uses IoP credentials to generate an authentication token for the device
  auto authenticate(std::string_view username, std::string_view password, const Api &api) const noexcept -> std::optional<AuthToken>;
  auto statusToString(const driver::StationStatus status) const noexcept -> std::optional<iop::StaticString>;

private:
  void handleNotConnected() noexcept;
  void handleInterrupt(const InterruptEvent event, const std::optional<std::reference_wrapper<const AuthToken>> &maybeToken) const noexcept;
  void handleIopCredentials() noexcept;
  void handleCredentials() noexcept;
  void handleMeasurements(const AuthToken &token) noexcept;

public:
  explicit EventLoop(iop::StaticString uri, iop::LogLevel logLevel_) noexcept
      : credentialsServer(logLevel_),
        api_(std::move(uri), logLevel_),
        logger(logLevel_, IOP_STATIC_STR("LOOP")), storage_(logLevel_),
        sensors(config::soilResistivityPower, config::soilTemperature, config::airTempAndHumidity, config::dhtVersion),
        nextMeasurement(0), nextYieldLog(0), nextNTPSync(0), nextHandleConnectionLost(0),
        nextTryStorageWifiCredentials(0), nextTryHardcodedWifiCredentials(0), nextTryHardcodedIopCredentials(0) {
    IOP_TRACE();
  }
  ~EventLoop() noexcept = default;
  auto operator=(EventLoop const &other) noexcept -> EventLoop & = delete;
  auto operator=(EventLoop &&other) noexcept -> EventLoop & = default;
  EventLoop(EventLoop const &other) noexcept = delete;
  EventLoop(EventLoop &&other) noexcept = default;
};

extern EventLoop eventLoop;

class GlobalData {
  struct StackStruct {
    std::array<char, 768> text;
    AuthToken token;
    std::array<char, 64> psk;
    std::array<char, 32> ssid;
  } *data;
  
  //static_assert(sizeof(StackStruct) <= 4096);

public:
  GlobalData() noexcept: data(new (std::nothrow) StackStruct()) {
    iop_assert(data, IOP_STATIC_STR("Unable to allocate buffer"));
    memset((void*)data, 0, sizeof(StackStruct));
  }
  void reset() noexcept {
    memset((void*)data, 0, sizeof(StackStruct));
  }
  //GlobalData() noexcept: data(reinterpret_cast<StackStruct *>(0x3FFFE000)) {
  //  memset((void*)0x3FFFE000, 0, 4096);
  //}
  //void reset() noexcept {
  //  memset((void*)0x3FFFE000, 0, 4096);
  //}
  auto psk() noexcept -> std::array<char, 64> & {
    return this->data->psk;
  }
  auto ssid() noexcept -> std::array<char, 32> & {
    return this->data->ssid;
  }
  auto token() noexcept -> AuthToken & {
    return this->data->token;
  }
  auto text() noexcept -> std::array<char, 768> & {
    return this->data->text;
  }
  // ...
};
extern GlobalData globalData;

#endif