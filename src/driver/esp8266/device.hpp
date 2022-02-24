#include "driver/device.hpp"
#include "driver/network.hpp"
#include "driver/panic.hpp"
#include "sys/pgmspace.h"
#include "ESP8266HTTPClient.h"
#include "ESP8266WiFi.h"
#include <umm_malloc/umm_heap_select.h>

namespace driver {
auto Device::syncNTP() const noexcept -> void {
  // UTC by default, should we change according to the user? We currently only use this to validate SSL cert dates
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}
auto Device::platform() const noexcept -> iop::StaticString {
  return IOP_STR("ESP8266");
}
auto Device::vcc() const noexcept -> uint16_t {
    return ESP.getVcc();
}
auto Device::availableStorage() const noexcept -> uintmax_t {
    return ESP.getFreeSketchSpace();
}

auto Device::availableMemory() const noexcept -> Memory {
  static std::map<std::string_view, uintmax_t> heap;
  static std::map<std::string_view, uintmax_t> biggestBlock;

  if (heap.size() == 0) {
    {
      HeapSelectDram _guard;
      heap.insert({ std::string_view("DRAM"), ESP.getFreeHeap() });
      biggestBlock.insert({ std::string_view("DRAM"), ESP.getMaxFreeBlockSize() });
    }
    {
      HeapSelectIram _guard;
      heap.insert({ std::string_view("IRAM"), ESP.getFreeHeap() });
      biggestBlock.insert({ std::string_view("IRAM"), ESP.getMaxFreeBlockSize() });
    }
  }

  ESP.resetFreeContStack();
  return Memory(ESP.getFreeContStack(), heap, biggestBlock);
}
auto Device::deepSleep(const uintmax_t seconds) const noexcept -> void {
  ESP.deepSleep(seconds * 1000000);
  
  // Let's allow the wifi to reconnect
  ::WiFi.forceSleepWake();
  ::WiFi.mode(WIFI_STA);
  ::WiFi.reconnect();
  ::WiFi.waitForConnectResult();
}
auto Device::firmwareMD5() const noexcept -> iop::MD5Hash & {
  static auto md5 = std::optional<iop::MD5Hash>();
  if (md5)
    return *md5;
    
  IOP_TRACE();

  // Copied verbatim from ESP code
  uint32_t lengthLeft = ESP.getSketchSize();

  size_t bufSize = 1024;
  std::unique_ptr<uint8_t[]> buf;
  uint32_t offset = 0;

  while (!buf) {
    bufSize /= 2;
    buf = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[bufSize]);
  }
  iop_assert(bufSize > 128, IOP_STR("Too low on RAM to generate MD5 hash"));
  
  MD5Builder builder;
  builder.begin();
  while (lengthLeft > 0) {
      size_t readBytes = (lengthLeft < bufSize) ? lengthLeft : bufSize;

      auto *ptr = reinterpret_cast<uint32_t*>(buf.get());
      auto alignedSize = (readBytes + 3) & ~3;
      
      const auto flashReadFailed = IOP_STR("Failed to read from flash to calculate MD5");
      // What can we do here?
      iop_assert(ESP.flashRead(offset, ptr, alignedSize) == SPI_FLASH_RESULT_OK, flashReadFailed);

      builder.add(buf.get(), readBytes);
      lengthLeft -= readBytes;
      offset += readBytes;
  }
  builder.calculate();

  md5 = iop::MD5Hash();
  builder.getChars(md5->data());
  return *md5;
}
auto Device::macAddress() const noexcept -> iop::MacAddress & {
  static auto mac = std::optional<iop::MacAddress>();
  if (mac)
    return *mac;

  IOP_TRACE();

  std::array<uint8_t, 6> buff = {0};
  wifi_get_macaddr(STATION_IF, buff.data());

  mac = iop::MacAddress();
  
  const auto *fmt = IOP_STORAGE_RAW("%02X:%02X:%02X:%02X:%02X:%02X");
  sprintf_P(mac->data(), fmt, buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
  return *mac;
}
}