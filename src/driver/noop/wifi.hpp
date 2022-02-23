#include "driver/wifi.hpp"

namespace driver { 
Wifi::Wifi() noexcept {}
Wifi::~Wifi() noexcept {}
void Wifi::onConnect(std::function<void()> f) noexcept { (void) f; }
Wifi::Wifi(Wifi &&other) noexcept { (void) other; }
auto Wifi::operator=(Wifi &&other) noexcept -> Wifi & { (void) other; return *this; }
StationStatus Wifi::status() const noexcept { return StationStatus::GOT_IP; }
void Wifi::setupAccessPoint() const noexcept {}
std::string Wifi::APIP() const noexcept { return "127.0.0.1"; }
void Wifi::setMode(WiFiMode mode) const noexcept { (void) mode; }
void Wifi::reconnect() const noexcept {}
std::string Wifi::localIP() const noexcept { return "127.0.0.1"; }
void Wifi::stationDisconnect() const noexcept {}
std::pair<iop::NetworkName, iop::NetworkPassword> Wifi::credentials() const noexcept {
    static iop::NetworkName ssid;
    ssid.fill('\0');
    ssid = { 'S', 'S', 'I', 'D' };
    static iop::NetworkPassword psk;
    psk.fill('\0');
    psk = { 'P', 'S', 'K' };
    return std::make_pair(ssid, psk);
}
void Wifi::wake() const noexcept {}
void Wifi::setup(driver::CertStore *certStore) noexcept { (void) certStore; }
WiFiMode Wifi::mode() const noexcept { return WiFiMode::STATION; }

void Wifi::connectAP(std::string_view ssid, std::string_view psk) const noexcept { (void) ssid; (void) psk; }
bool Wifi::begin(std::string_view ssid, std::string_view psk) const noexcept { (void) ssid; (void) psk; return true; }
}