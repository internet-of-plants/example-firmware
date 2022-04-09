#ifndef IOP_LOOP
#define IOP_LOOP
#include "api.hpp"

#include "iop/device.hpp"
#include "iop/thread.hpp"
#include "iop/network.hpp"
#include "iop/panic.hpp"
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

  iop::time::milliseconds nextMeasurement;
  iop::time::milliseconds nextYieldLog;
  iop::time::milliseconds nextNTPSync;
  iop::time::milliseconds nextHandleConnectionLost;

  iop::time::milliseconds nextTryStorageWifiCredentials;
  iop::time::milliseconds nextTryHardcodedWifiCredentials;
  iop::time::milliseconds nextTryHardcodedIopCredentials;

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
  void handleInterrupt(const InterruptEvent event, const std::optional<std::reference_wrapper<const AuthToken>> &token) const noexcept;
  void handleIopCredentials() noexcept;
  void handleCredentials() noexcept;
  void handleMeasurements(const AuthToken &token) noexcept;

public:
  explicit EventLoop(iop::StaticString uri, iop::LogLevel logLevel_) noexcept
      : credentialsServer(logLevel_),
        api_(std::move(uri), logLevel_),
        logger(logLevel_, IOP_STR("LOOP")), storage_(logLevel_),
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

#endif