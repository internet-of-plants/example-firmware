
#include "driver/wifi.hpp"
#include "driver/log.hpp"

namespace driver {
Wifi::Wifi() noexcept {}
Wifi::~Wifi() noexcept {}
StationStatus Wifi::status() const noexcept {
    return StationStatus::GOT_IP;
}
void Wifi::stationDisconnect() const noexcept {}
std::pair<std::array<char, 32>, std::array<char, 64>> Wifi::credentials() const noexcept {
  IOP_TRACE()
  std::array<char, 32> ssid;
  ssid.fill('\0');
  memcpy(ssid.data(), "SSID", 4);
  
  std::array<char, 64> psk;
  psk.fill('\0');
  memcpy(psk.data(), "PSK", 3);
  
  return std::make_pair(ssid, psk);
}
std::string Wifi::localIP() const noexcept {
    return "127.0.0.1";
}
std::string Wifi::APIP() const noexcept {
    return "127.0.0.1";
}
void Wifi::connectAP(std::string_view ssid, std::string_view psk) const noexcept {
    (void) ssid;
    (void) psk;
}
bool Wifi::begin(std::string_view ssid, std::string_view psk) const noexcept {
    (void) ssid;
    (void) psk;
    return true;
}
WiFiMode Wifi::mode() const noexcept {
    return WiFiMode::STATION;
}
void Wifi::wake() const noexcept {}
void Wifi::reconnect() const noexcept {}
void Wifi::setup(driver::CertStore *certStore) noexcept { (void)certStore; }
void Wifi::setMode(WiFiMode mode) const noexcept { (void) mode; }
void Wifi::onStationGotIP(std::function<void()> f) noexcept { (void) f; }

void Wifi::setupAccessPoint() const noexcept {} // NOOP
}