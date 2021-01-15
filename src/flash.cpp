#include "flash.hpp"

#ifndef IOP_FLASH_DISABLED
#include "EEPROM.h"

// Flags to check if information is written to flash
// chosen by fair dice roll, garanteed to be random
const uint8_t usedWifiConfigEEPROMFlag = 126;
const uint8_t usedAuthTokenEEPROMFlag = 127;
const uint8_t usedPlantIdEEPROMFlag = 128;

// Indexes, so each function know where they can write to.
// It's kinda bad, but for now it works (TODO: maybe use FS.h?)
// There is always 1 bit used for the 'isWritten' flag
const uint8_t wifiConfigIndex = 0;
const uint8_t wifiConfigSize = 1 + NetworkName::size + NetworkPassword::size;

const uint8_t authTokenIndex = wifiConfigIndex + wifiConfigSize;
const uint8_t authTokenSize = 1 + AuthToken::size;

const uint8_t plantIdIndex = authTokenIndex + authTokenSize;
// const uint8_t plantTokenSize = 1 + PlantId::size;

constexpr const uint16_t EEPROM_SIZE = 512;

auto Flash::setup() noexcept -> void { EEPROM.begin(EEPROM_SIZE); }

static auto constPtr() noexcept -> const char * {
  return reinterpret_cast<const char *>(EEPROM.getConstDataPtr());
}

auto Flash::readPlantId() const noexcept -> Option<PlantId> {
  this->logger.debug(F("Reading PlantId from Flash"));

  if (EEPROM.read(plantIdIndex) != usedPlantIdEEPROMFlag)
    return Option<PlantId>();

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto ptr = constPtr() + plantIdIndex + 1;
  const auto id = PlantId::fromStringTruncating(UnsafeRawString(ptr));
  this->logger.debug(F("Plant id found: "), id.asString());
  return Option<PlantId>(id);
}

void Flash::removePlantId() const noexcept {
  this->logger.debug(F("Deleting stored plant id"));
  if (EEPROM.read(plantIdIndex) == usedPlantIdEEPROMFlag) {
    EEPROM.write(plantIdIndex, 0);
    EEPROM.commit();
  }
}

void Flash::writePlantId(const PlantId &id) const noexcept {
  this->logger.debug(F("Writing plant id to storage: "), id.asString());
  EEPROM.write(plantIdIndex, usedPlantIdEEPROMFlag);
  EEPROM.put(plantIdIndex + 1, *id.asSharedArray());
  EEPROM.commit();
}

auto Flash::readAuthToken() const noexcept -> Option<AuthToken> {
  this->logger.debug(F("Reading AuthToken from Flash"));
  if (EEPROM.read(authTokenIndex) != usedAuthTokenEEPROMFlag)
    return Option<AuthToken>();

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto ptr = constPtr() + authTokenSize + 1;
  const auto token = AuthToken::fromStringTruncating(UnsafeRawString(ptr));
  this->logger.debug(F("Auth token found: "), token.asString());
  return Option<AuthToken>(token);
}

void Flash::removeAuthToken() const noexcept {
  this->logger.debug(F("Deleting stored auth token"));
  if (EEPROM.read(authTokenIndex) == usedAuthTokenEEPROMFlag) {
    EEPROM.write(authTokenIndex, 0);
    EEPROM.commit();
  }
}

void Flash::writeAuthToken(const AuthToken &token) const noexcept {
  this->logger.debug(F("Writing auth token to storage: "), token.asString());
  EEPROM.write(authTokenIndex, usedAuthTokenEEPROMFlag);
  EEPROM.put(authTokenSize + 1, *token.asSharedArray());
  EEPROM.commit();
}

auto Flash::readWifiConfig() const noexcept -> Option<WifiCredentials> {
  this->logger.debug(F("Reading WifiCredentials from Flash"));

  if (EEPROM.read(wifiConfigIndex) != usedWifiConfigEEPROMFlag)
    return Option<WifiCredentials>();

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto ptr = constPtr() + wifiConfigIndex + 1;
  const auto ssid = NetworkName::fromStringTruncating(UnsafeRawString(ptr));

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto pskRaw = UnsafeRawString(ptr + NetworkName::size);
  const auto psk = NetworkPassword::fromStringTruncating(pskRaw);

  this->logger.debug(F("Found network credentials: "), ssid.asString());
  return (WifiCredentials){.ssid = ssid, .password = psk};
}

void Flash::removeWifiConfig() const noexcept {
  this->logger.debug(F("Deleting stored wifi config"));
  if (EEPROM.read(wifiConfigIndex) == usedWifiConfigEEPROMFlag) {
    EEPROM.write(wifiConfigIndex, 0);
    EEPROM.commit();
  }
}

void Flash::writeWifiConfig(const WifiCredentials &config) const noexcept {
  const auto ssidStr = config.ssid.asString();
  this->logger.debug(F("Writing network credentials to storage: "), ssidStr);
  const auto &psk = *config.password.asSharedArray();

  EEPROM.write(wifiConfigIndex, usedWifiConfigEEPROMFlag);
  EEPROM.put(wifiConfigIndex + 1, *config.ssid.asSharedArray());
  EEPROM.put(wifiConfigIndex + 1 + NetworkName::size, psk);
  EEPROM.commit();
}
#endif

#ifdef IOP_FLASH_DISABLED
void Flash::setup() const noexcept {}
Option<AuthToken> Flash::readAuthToken() const noexcept {
  return AuthToken::empty();
}
void Flash::removeAuthToken() const noexcept {}
void Flash::writeAuthToken(const AuthToken &token) const noexcept {
  (void)token;
}
Option<PlantId> Flash::readPlantId() const noexcept { return PlantId::empty(); }
void Flash::removePlantId() const noexcept {};
void Flash::writePlantId(const PlantId &id) const noexcept { (void)id; }
Option<struct WifiCredentials> Flash::readWifiConfig() const noexcept {
  return (struct WifiCredentials){
      .ssid = NetworkName(NetworkName::empty()),
      .password = NetworkPassword(NetworkPassword::empty()),
  };
}
void Flash::removeWifiConfig() const noexcept {}
void Flash::writeWifiConfig(const struct WifiCredentials &id) const noexcept {
  (void)id;
}
#endif