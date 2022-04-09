#include "loop.hpp" 
#include "iop/panic.hpp"
#include "iop/io.hpp"

EventLoop eventLoop(config::uri(), config::logLevel);

auto EventLoop::setup() noexcept -> void {
  IOP_TRACE();

  this->logger.info(IOP_STR("Start Setup"));
  driver::gpio.setMode(driver::io::LED_BUILTIN, driver::io::Mode::OUTPUT);

  Storage::setup();
  reset::setup();
  this->sensors.setup();
  this->api().setup();
  this->credentialsServer.setup();
  this->logger.info(IOP_STR("Setup finished"));
  this->logger.info(IOP_STR("MD5: "), iop::to_view(driver::device.firmwareMD5()));
}

constexpr static uint64_t intervalTryStorageWifiCredentialsMillis =
    60 * 60 * 1000; // 1 hour

constexpr static uint64_t intervalTryHardcodedWifiCredentialsMillis =
    60 * 60 * 1000; // 1 hour

constexpr static uint64_t intervalTryHardcodedIopCredentialsMillis =
    60 * 60 * 1000; // 1 hour

auto EventLoop::loop() noexcept -> void {
    this->logger.trace(IOP_STR("\n\n\n\n\n\n"));

    IOP_TRACE();

#ifdef LOG_MEMORY
    iop::logMemory(this->logger);
#endif

    const auto authToken = this->storage().token();

    // Handle all queued interrupts (only allows one of each kind concurrently)
    while (true) {
        auto ev = utils::descheduleInterrupt();
        if (ev == InterruptEvent::NONE)
          break;

        this->handleInterrupt(ev, authToken);
        driver::thisThread.yield();
    }
    const auto now = driver::thisThread.timeRunning();
    const auto isConnected = iop::Network::isConnected();

    if (isConnected && authToken)
        this->credentialsServer.close();

    if (isConnected && this->nextNTPSync < now) {
      this->logger.info(IOP_STR("Syncing NTP"));

      driver::device.syncNTP();
      
      this->logger.info(IOP_STR("Time synced"));

      constexpr const uint32_t sixHours = 6 * 60 * 60 * 1000;
      this->nextNTPSync = now + sixHours;

    } else if (isConnected && !authToken) {
        this->handleIopCredentials();

    } else if (!isConnected) {
        this->handleNotConnected();

    } else if (this->nextMeasurement <= now) {
      this->nextHandleConnectionLost = 0;
      this->nextMeasurement = now + config::interval;
      iop_assert(authToken, IOP_STR("Auth Token not found"));
      this->handleMeasurements(*authToken);
        
    } else if (this->nextYieldLog <= now) {
      this->nextHandleConnectionLost = 0;
      this->logger.trace(IOP_STR("Waiting"));

      constexpr const uint16_t tenSeconds = 10000;
      this->nextYieldLog = now + tenSeconds;

    } else {
      this->nextHandleConnectionLost = 0;
    }
}

auto EventLoop::handleNotConnected() noexcept -> void {
  IOP_TRACE();

  // If connection is lost frequently we open the credentials server, to
  // allow replacing the wifi credentials. Since we only remove it
  // from storage if it's going to be replaced by a new one (allows
  // for more resiliency) - or during factory reset
  constexpr const uint32_t oneMinute = 60 * 1000;

  const auto now = driver::thisThread.timeRunning();
  const auto &wifi = this->storage().wifi();
  const auto isConnected = iop::Network::isConnected();

  if (!isConnected && wifi && this->nextTryStorageWifiCredentials <= now) {
    const auto &stored = wifi->get();
    const auto ssid = iop::to_view(stored.ssid.get());
    const auto psk = iop::to_view(stored.password.get());

    this->logger.info(IOP_STR("Trying wifi credentials stored in storage: "), iop::to_view(iop::scapeNonPrintable(ssid)));
    this->logger.debug(IOP_STR("Password:"), iop::to_view(iop::scapeNonPrintable(psk)));
    this->connect(ssid, psk);

    // WiFi Credentials stored in persistent memory
    //
    // Ideally it won't be wrong, but we can't know if it's wrong or if the router just is offline
    // So we don't delete it and prefer, retrying later
    this->nextTryStorageWifiCredentials = now + intervalTryStorageWifiCredentialsMillis;

  } else if (!isConnected && config::wifiNetworkName() && config::wifiPassword() && this->nextTryHardcodedWifiCredentials <= now) { 
    this->logger.info(IOP_STR("Trying hardcoded wifi credentials"));

    const auto ssid = *config::wifiNetworkName();
    const auto psk = *config::wifiPassword();
    this->connect(ssid.toString(), psk.toString());

    // WiFi Credentials hardcoded at "configuration.hpp"
    //
    // Ideally it won't be wrong, but we can't know if it's wrong or if the router just is offline
    // So we keep retrying
    this->nextTryHardcodedWifiCredentials = now + intervalTryHardcodedWifiCredentialsMillis;

  } else if (this->nextHandleConnectionLost < now) {
    // If network is offline for a while we open the captive portal to collect new wifi credentials
    this->logger.debug(IOP_STR("Has creds, but no signal, opening server"));
    this->nextHandleConnectionLost = now + oneMinute;
    this->handleCredentials();

  } else {
    // No-op, we must just wait
  }
}

auto EventLoop::handleIopCredentials() noexcept -> void {
  IOP_TRACE();

  const auto now = driver::thisThread.timeRunning();

  if (config::iopEmail() && config::iopPassword() && this->nextTryHardcodedIopCredentials <= now) {
    this->nextTryHardcodedIopCredentials = now + intervalTryHardcodedIopCredentialsMillis;

    this->logger.info(IOP_STR("Trying hardcoded iop credentials"));

    const auto email = *config::iopEmail();
    const auto password = *config::iopPassword();
    const auto tok = this->authenticate(email.toString(), password.toString(), this->api());
    if (tok)
      this->storage().setToken(*tok);
  } else {
    this->handleCredentials();
  }
}

auto EventLoop::handleInterrupt(const InterruptEvent event, const std::optional<std::reference_wrapper<const AuthToken>> &token) const noexcept -> void {
    // Satisfies linter when all interrupt features are disabled
    (void)*this;
    (void)event;

    IOP_TRACE();

    switch (event) {
    case InterruptEvent::NONE:
      break;
    case InterruptEvent::FACTORY_RESET:
#ifdef IOP_FACTORY_RESET
      this->logger.warn(IOP_STR("Factory Reset: deleting stored credentials"));
      this->storage().removeWifi();
      this->storage().removeToken();
      iop::Network::disconnect();
#endif
      (void)0; // Satisfies linter
      break;
    case InterruptEvent::MUST_UPGRADE:
#ifdef IOP_OTA
      if (token) {
        const auto status = this->api().upgrade(*token);
        switch (status) {
        case driver::UpgradeStatus::FORBIDDEN:
          this->logger.warn(IOP_STR("Invalid auth token, but keeping since at OTA"));
          return;

        case driver::UpgradeStatus::BROKEN_CLIENT:
          iop_panic(IOP_STR("Api::upgrade internal buffer overflow"));

        // Already logged at the network level
        case driver::UpgradeStatus::IO_ERROR:
        case driver::UpgradeStatus::BROKEN_SERVER:
          // Nothing to be done besides retrying later

        case driver::UpgradeStatus::NO_UPGRADE: // Shouldn't happen but ok
          return;
        }

        this->logger.error(IOP_STR("Bad status, EventLoop::handleInterrupt"));
      } else {
        this->logger.error(IOP_STR("Upgrade expected, but no auth token available"));
      }
#endif
      (void)1; // Satisfies linter
      break;
    case InterruptEvent::ON_CONNECTION:
#ifdef IOP_ONLINE
      const auto status = this->statusToString(iop::wifi.status());
      this->logger.debug(IOP_STR("WiFi connected: "), status.value_or(IOP_STR("BadData")));

      const auto [ssid, psk] = iop::wifi.credentials();

      this->logger.info(IOP_STR("Connected to network: "), iop::to_view(iop::scapeNonPrintable(iop::to_view(ssid))));
      this->storage().setWifi(WifiCredentials(ssid, psk));
#endif
      (void)2; // Satisfies linter
      break;
    };
}

auto EventLoop::handleCredentials() noexcept -> void {
    IOP_TRACE();

    const auto token = this->credentialsServer.serve(this->api());
    if (token)
      this->storage().setToken(*token);
}

auto EventLoop::handleMeasurements(const AuthToken &token) noexcept -> void {
    IOP_TRACE();

    this->logger.debug(IOP_STR("Handle Measurements"));

    const auto measurements = sensors.measure();
    const auto status = this->api().registerEvent(token, measurements);

    switch (status) {
    case iop::NetworkStatus::FORBIDDEN:
      this->logger.error(IOP_STR("Unable to send measurements"));
      this->logger.warn(IOP_STR("Auth token was refused, deleting it"));
      this->storage().removeToken();
      return;

    case iop::NetworkStatus::BROKEN_CLIENT:
      this->logger.error(IOP_STR("Unable to send measurements"));
      iop_panic(IOP_STR("Api::registerEvent internal buffer overflow"));

    // Already logged at the Network level
    case iop::NetworkStatus::BROKEN_SERVER:
    case iop::NetworkStatus::IO_ERROR:
      // Nothing to be done besides retrying later

    case iop::NetworkStatus::OK: // Cool beans
      return;
    }

    this->logger.error(IOP_STR("Unexpected status, EventLoop::handleMeasurements"));
}


auto EventLoop::connect(std::string_view ssid, std::string_view password) const noexcept -> void {
  IOP_TRACE();
  this->logger.info(IOP_STR("Connect: "), ssid);

  if (!iop::wifi.connectToAccessPoint(ssid, password)) {
    this->logger.error(IOP_STR("Wifi authentication timed out"));
    return;
  }

  if (!iop::Network::isConnected()) {
    const auto status = iop::wifi.status();
    auto statusStr = this->statusToString(status);
    if (!statusStr)
      return; // It already will be logged by statusToString;

    this->logger.error(IOP_STR("Invalid wifi credentials ("), *statusStr, IOP_STR("): "), std::move(ssid));
  }
}

auto EventLoop::authenticate(std::string_view username, std::string_view password, const Api &api) const noexcept -> std::optional<AuthToken> {
  IOP_TRACE();

  iop::wifi.setMode(driver::WiFiMode::STATION);
  auto authToken = api.authenticate(username, std::move(password));
  iop::wifi.setMode(driver::WiFiMode::ACCESS_POINT_AND_STATION);

  this->logger.info(IOP_STR("Tried to authenticate"));
  if (const auto *error = std::get_if<iop::NetworkStatus>(&authToken)) {
    const auto &status = *error;

    switch (status) {
    case iop::NetworkStatus::FORBIDDEN:
      this->logger.error(IOP_STR("Invalid IoP credentials: "), username);
      return std::nullopt;

    case iop::NetworkStatus::BROKEN_CLIENT:
      iop_panic(IOP_STR("CredentialsServer::authenticate internal buffer overflow"));

    // Already logged at the Network level
    case iop::NetworkStatus::IO_ERROR:
    case iop::NetworkStatus::BROKEN_SERVER:
      // Nothing to be done besides retrying later
      return std::nullopt;

    case iop::NetworkStatus::OK:
      // On success an AuthToken is returned, not OK
      iop_panic(IOP_STR("Unreachable"));
    }

    this->logger.crit(IOP_STR("CredentialsServer::authenticate bad status"));
    return std::nullopt;

  } else if (auto *token = std::get_if<AuthToken>(&authToken)) {
    return *token;
  }

  iop_panic(IOP_STR("Invalid variant"));
}

auto EventLoop::statusToString(const driver::StationStatus status) const noexcept -> std::optional<iop::StaticString> {
  IOP_TRACE();

  switch (status) {
  case driver::StationStatus::IDLE:
    return IOP_STR("STATION_IDLE");

  case driver::StationStatus::CONNECTING:
    return IOP_STR("STATION_CONNECTING");

  case driver::StationStatus::WRONG_PASSWORD:
    return IOP_STR("STATION_WRONG_PASSWORD");

  case driver::StationStatus::NO_AP_FOUND:
    return IOP_STR("STATION_NO_AP_FOUND");

  case driver::StationStatus::CONNECT_FAIL:
    return IOP_STR("STATION_CONNECT_FAIL");

  case driver::StationStatus::  GOT_IP:
    return IOP_STR("STATION_GOT_IP");
  }

  this->logger.error(IOP_STR("Unknown status: ").toString() + std::to_string(static_cast<uint8_t>(status)));
  return std::nullopt;
}