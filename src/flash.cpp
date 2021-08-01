#include "flash.hpp"
#include "loop.hpp"

#ifndef IOP_FLASH_DISABLED
#include "driver/flash.hpp"
#include "core/panic.hpp"

constexpr const uint16_t EEPROM_SIZE = 512;

// If another type is to be written to flash be carefull not to mess with what
// already is there and update the static_assert below. Same deal for removing
// types.

// Magic bytes. Flags to check if information is written to flash.
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

auto Flash::setup() noexcept -> void { driver::flash.setup(EEPROM_SIZE); }

auto Flash::readAuthToken() const noexcept -> std::optional<std::reference_wrapper<const AuthToken>> {
  IOP_TRACE();

  // Check if magic byte is set in flash (as in, something is stored)
  if (driver::flash.read(authTokenIndex) != usedAuthTokenEEPROMFlag)
    return std::optional<std::reference_wrapper<const AuthToken>>();

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const uint8_t *ptr = driver::flash.asRef() + authTokenIndex + 1;
  
  memcpy(unused4KbSysStack.token().data(), ptr, 64);

  const auto tok = iop::to_view(unused4KbSysStack.token());
  // AuthToken must be printable US-ASCII (to be stored in HTTP headers))
  if (!iop::isAllPrintable(tok) || tok.length() != 64) {
    this->logger.error(F("Auth token was non printable: "), iop::to_view(iop::scapeNonPrintable(tok)));
    this->removeAuthToken();
    return std::optional<std::reference_wrapper<const AuthToken>>();
  }

  this->logger.trace(F("Found Auth token: "), tok);

  return std::make_optional(std::ref(unused4KbSysStack.token()));
}

void Flash::removeAuthToken() const noexcept {
  IOP_TRACE();

  unused4KbSysStack.token().fill('\0');

  // Checks if it's written to flash first, avoids wasting writes
  if (driver::flash.read(authTokenIndex) == usedAuthTokenEEPROMFlag) {
    this->logger.info(F("Deleting stored auth token"));

    // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
    memset(driver::flash.asMut() + authTokenIndex, 0, authTokenSize);
    driver::flash.commit();
  }
}

void Flash::writeAuthToken(const AuthToken &token) const noexcept {
  IOP_TRACE();

  // Avoids re-writing same data
  const size_t size = sizeof(AuthToken);
  const auto *tok = driver::flash.asRef() + authTokenIndex + 1;  
  if (memcmp(tok, token.begin(), size) == 0) {
    this->logger.debug(F("Auth token already stored in flash"));
    return;
  }

  this->logger.info(F("Writing auth token to storage: "), iop::to_view(token));
  driver::flash.write(authTokenIndex, usedAuthTokenEEPROMFlag);
  driver::flash.put(authTokenIndex + 1, token);
  driver::flash.commit();
}

auto Flash::readWifiConfig() const noexcept -> std::optional<std::reference_wrapper<const WifiCredentials>> {
  IOP_TRACE();

  // Check if magic byte is set in flash (as in, something is stored)
  if (driver::flash.read(wifiConfigIndex) != usedWifiConfigEEPROMFlag)
    return std::optional<std::reference_wrapper<const WifiCredentials>>();

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto *ptr = driver::flash.asRef() + wifiConfigIndex + 1;

  // We treat wifi credentials as a blob instead of worrying about encoding

  memcpy(unused4KbSysStack.ssid().data(), ptr, 32);
  memcpy(unused4KbSysStack.psk().data(), ptr + 32, 64);

  const auto ssidStr = iop::scapeNonPrintable(std::string_view(unused4KbSysStack.ssid().data(), 32));
  this->logger.trace(F("Found network credentials: "), iop::to_view(ssidStr));
  const auto creds = WifiCredentials(unused4KbSysStack.ssid(), unused4KbSysStack.psk());
  return std::make_optional(std::ref(creds));
}

void Flash::removeWifiConfig() const noexcept {
  IOP_TRACE();
  this->logger.info(F("Deleting stored wifi config"));

  unused4KbSysStack.ssid().fill('\0');
  unused4KbSysStack.psk().fill('\0');

  // Checks if it's written to flash first, avoids wasting writes
  if (driver::flash.read(wifiConfigIndex) == usedWifiConfigEEPROMFlag) {
    // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
    memset(driver::flash.asMut() + wifiConfigIndex, 0, wifiConfigSize);
    driver::flash.commit();
  }
}

void Flash::writeWifiConfig(const WifiCredentials &config) const noexcept {
  IOP_TRACE();

  // Avoids re-writing same data
  const size_t ssidSize = sizeof(NetworkName);
  const size_t pskSize = sizeof(NetworkPassword);
  const auto *ssid = driver::flash.asRef() + wifiConfigIndex + 1;  

  const auto sameSSID = memcmp(ssid, &config.ssid.get(), ssidSize) == 0;
  const auto samePSK = memcmp(ssid + ssidSize, &config.password.get(), pskSize) == 0;
  if (sameSSID && samePSK) {
    this->logger.debug(F("WiFi credentials already stored in flash"));
    return;
  }

  this->logger.info(F("Writing wifi credentials to storage: "), iop::to_view(config.ssid, ssidSize));
  driver::flash.write(wifiConfigIndex, usedWifiConfigEEPROMFlag);
  driver::flash.put(wifiConfigIndex + 1, config.ssid);
  driver::flash.put(wifiConfigIndex + 1 + 32, config.password);
  driver::flash.commit();
}
#endif

#ifdef IOP_FLASH_DISABLED
void Flash::setup() noexcept { IOP_TRACE(); }
auto Flash::readAuthToken() const noexcept -> std::optional<std::reference_wrapper<const AuthToken>> {
  (void)*this;
  IOP_TRACE();
  return std::make_optional(AuthToken(unused4KbSysStack.token()));
}
void Flash::removeAuthToken() const noexcept {
  (void)*this;
  IOP_TRACE();
}
void Flash::writeAuthToken(const AuthToken &token) const noexcept {
  (void)*this;
  IOP_TRACE();
  (void)token;
}
void Flash::removeWifiConfig() const noexcept {
  (void)*this;
  IOP_TRACE();
}

auto Flash::readWifiConfig() const noexcept -> std::optional<std::reference_wrapper<const WifiCredentials>> {
  (void)*this;
  IOP_TRACE();
  return std::make_optional(WifiCredentials(unused4KbSysStack.ssid(), unused4KbSysStack.psk()));
}
void Flash::writeWifiConfig(const WifiCredentials &config) const noexcept {
  (void)*this;
  IOP_TRACE();
  (void)config;
}
#endif