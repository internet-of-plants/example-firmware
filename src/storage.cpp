#include "storage.hpp"
#include "loop.hpp"

#ifndef IOP_STORAGE_DISABLED
#include "driver/storage.hpp"
#include "driver/panic.hpp"

constexpr const uint16_t EEPROM_SIZE = 512;

// If another type is to be written to storage be carefull not to mess with what
// already is there and update the static_assert below. Same deal for removing
// types.

// Magic bytes. Flags to check if information is written to storage.
// Chosen by fair dice roll, garanteed to be random
const uint8_t usedWifiConfigEEPROMFlag = 125;
const uint8_t usedAuthTokenEEPROMFlag = 126;

// One byte is reserved for the magic byte ('isWritten' flag)
const uint16_t authTokenSize = 1 + 64;
const uint16_t wifiConfigSize = 1 + 32 + 64;

// Allows each method to know where to write
const uint16_t wifiConfigIndex = 0;
const uint16_t authTokenIndex = wifiConfigIndex + wifiConfigSize;

static_assert(authTokenIndex + authTokenSize < EEPROM_SIZE,
              "EEPROM too small to store needed credentials");

auto Storage::setup() noexcept -> void { driver::storage.setup(EEPROM_SIZE); }
auto Storage::token() const noexcept -> std::optional<std::reference_wrapper<const AuthToken>> {
  IOP_TRACE();

  // Check if magic byte is set in storage (as in, something is stored)
  if (driver::storage.read(authTokenIndex) != usedAuthTokenEEPROMFlag)
    return std::nullopt;

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const uint8_t *ptr = driver::storage.asRef() + authTokenIndex + 1;
  
  memcpy(globalData.token().data(), ptr, 64);

  const auto tok = iop::to_view(globalData.token());
  // AuthToken must be printable US-ASCII (to be stored in HTTP headers))
  if (!iop::isAllPrintable(tok) || tok.length() != 64) {
    this->logger.error(IOP_STATIC_STRING("Auth token was non printable: "), iop::to_view(iop::scapeNonPrintable(tok)));
    this->removeToken();
    return std::nullopt;
  }

  this->logger.trace(IOP_STATIC_STRING("Found Auth token: "), tok);

  const auto & ret = globalData.token();
  return std::ref(ret);
}

void Storage::removeToken() const noexcept {
  IOP_TRACE();

  globalData.token().fill('\0');

  // Checks if it's written to storage first, avoids wasting writes
  if (driver::storage.read(authTokenIndex) == usedAuthTokenEEPROMFlag) {
    this->logger.info(IOP_STATIC_STRING("Deleting stored auth token"));

    // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
    memset(driver::storage.asMut() + authTokenIndex, 0, authTokenSize);
    driver::storage.commit();
  }
}

void Storage::setToken(const AuthToken &token) const noexcept {
  IOP_TRACE();

  // Avoids re-writing same data
  const size_t size = sizeof(AuthToken);
  const auto *tok = driver::storage.asRef() + authTokenIndex + 1;  
  if (memcmp(tok, token.begin(), size) == 0) {
    this->logger.debug(IOP_STATIC_STRING("Auth token already stored in storage"));
    return;
  }

  this->logger.info(IOP_STATIC_STRING("Writing auth token to storage: "), iop::to_view(token));
  driver::storage.write(authTokenIndex, usedAuthTokenEEPROMFlag);
  driver::storage.put(authTokenIndex + 1, token);
  driver::storage.commit();
}

auto Storage::wifi() const noexcept -> std::optional<std::reference_wrapper<const WifiCredentials>> {
  IOP_TRACE();

  // Check if magic byte is set in storage (as in, something is stored)
  if (driver::storage.read(wifiConfigIndex) != usedWifiConfigEEPROMFlag)
    return std::nullopt;

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto *ptr = driver::storage.asRef() + wifiConfigIndex + 1;

  // We treat wifi credentials as a blob instead of worrying about encoding

  memcpy(globalData.ssid().data(), ptr, 32);
  memcpy(globalData.psk().data(), ptr + 32, 64);

  const auto ssidStr = iop::scapeNonPrintable(std::string_view(globalData.ssid().data(), 32));
  this->logger.trace(IOP_STATIC_STRING("Found network credentials: "), iop::to_view(ssidStr));
  const auto creds = WifiCredentials(globalData.ssid(), globalData.psk());
  return std::make_optional(std::ref(creds));
}

void Storage::removeWifi() const noexcept {
  IOP_TRACE();
  this->logger.info(IOP_STATIC_STRING("Deleting stored wifi config"));

  globalData.ssid().fill('\0');
  globalData.psk().fill('\0');

  // Checks if it's written to storage first, avoids wasting writes
  if (driver::storage.read(wifiConfigIndex) == usedWifiConfigEEPROMFlag) {
    // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
    memset(driver::storage.asMut() + wifiConfigIndex, 0, wifiConfigSize);
    driver::storage.commit();
  }
}

void Storage::setWifi(const WifiCredentials &config) const noexcept {
  IOP_TRACE();

  // Avoids re-writing same data
  const size_t ssidSize = sizeof(NetworkName);
  const size_t pskSize = sizeof(NetworkPassword);
  const auto *ssid = driver::storage.asRef() + wifiConfigIndex + 1;  

  // Theoretically SSIDs can have a nullptr inside of it, but currently ESP8266 gives us random garbage after the first '\0' instead of zeroing the rest
  // So we do not accept SSIDs with a nullptr in the middle
  const auto sameSSID = strncmp(reinterpret_cast<const char*>(ssid), config.ssid.get().begin(), ssidSize) == 0;
  const auto samePSK = strncmp(reinterpret_cast<const char*>(ssid + ssidSize), config.password.get().begin(), pskSize) == 0;
  if (sameSSID && samePSK) {
    this->logger.debug(IOP_STATIC_STRING("WiFi credentials already stored in storage"));
    return;
  }

  this->logger.info(IOP_STATIC_STRING("Writing wifi credentials to storage: "), iop::to_view(config.ssid, ssidSize));
  this->logger.debug(IOP_STATIC_STRING("WiFi Creds: "), iop::to_view(config.ssid, ssidSize), IOP_STATIC_STRING(" "), iop::to_view(config.password, pskSize));

  driver::storage.write(wifiConfigIndex, usedWifiConfigEEPROMFlag);
  driver::storage.put(wifiConfigIndex + 1, config.ssid.get());
  driver::storage.put(wifiConfigIndex + 1 + 32, config.password.get());
  driver::storage.commit();
}
#endif

#ifdef IOP_STORAGE_DISABLED
void Storage::setup() noexcept { IOP_TRACE(); }
auto Storage::token() const noexcept -> std::optional<std::reference_wrapper<const AuthToken>> {
  (void)*this;
  IOP_TRACE();
  return {};
}
void Storage::removeToken() const noexcept {
  (void)*this;
  IOP_TRACE();
}
void Storage::setToken(const AuthToken &token) const noexcept {
  (void)*this;
  IOP_TRACE();
  (void)token;
}
void Storage::removeWifi() const noexcept {
  (void)*this;
  IOP_TRACE();
}

auto Storage::wifi() const noexcept -> std::optional<std::reference_wrapper<const WifiCredentials>> {
  (void)*this;
  IOP_TRACE();
  return {};
}
void Storage::setWifi(const WifiCredentials &config) const noexcept {
  (void)*this;
  IOP_TRACE();
  (void)config;
}
#endif