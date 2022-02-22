#include "driver/device.hpp"
#include "driver/runtime.hpp"
#include "driver/panic.hpp"

#include "driver/cpp17/md5.hpp"

#include <stdint.h>
#include <thread>
#include <filesystem>
#include <fstream>

// POSIX
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>

namespace driver {
// NOOP for now as we assume the host will do it for us, eventually we will want to do it by ourselves
void Device::syncNTP() const noexcept {}

auto Device::vcc() const noexcept -> uint16_t { return UINT16_MAX; }

auto Device::platform() const noexcept -> iop::StaticString { return IOP_STATIC_STR("POSIX"); }

auto Device::availableStorage() const noexcept -> uintmax_t {
  // TODO: handle errors
  std::error_code code;
  auto available = std::filesystem::space(std::filesystem::current_path(), code).available;
  if (code) return 0;
  return available;
}

auto Device::availableMemory() const noexcept -> Memory {
  // POSIX
  long pages = sysconf(_SC_PHYS_PAGES);
  long page_size = sysconf(_SC_PAGE_SIZE);

  std::map<std::string_view, uintmax_t> heap;
  heap.insert({ std::string_view("DRAM"), pages * page_size });

  std::map<std::string_view, uintmax_t> biggestBlock;
  biggestBlock.insert({ std::string_view("DRAM"), pages * page_size }); // Ballpark

  return Memory(driver::stack_used(), heap, biggestBlock);
}

void Device::deepSleep(uintmax_t seconds) const noexcept {
  if (seconds == 0) seconds = UINTMAX_MAX;
  // Note: this only sleeps our current thread, but it's not that useful sleeping the entire process in Posix
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

iop::MD5Hash & Device::firmwareMD5() const noexcept {
  static iop::MD5Hash hash;
  static bool cached = false;
  if (cached)
    return hash;
  
  const auto filename = std::filesystem::current_path().append(driver::execution_path());
  std::ifstream file(filename);
  iop_assert(file.is_open(), IOP_STATIC_STR("Unable to open firmware file"));

  const auto data = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  iop_assert(!file.fail(), IOP_STATIC_STR("Unable to read from firmware file"));

  file.close();
  iop_assert(!file.fail(), IOP_STATIC_STR("Unable to close firmware file"));

  uint8_t buff[16];
  MD5_CTX md5;
  MD5_Init(&md5);
  MD5_Update(&md5, &data.front(), data.size());
  MD5_Final(buff, &md5);
  for (uint8_t i = 0; i < 16; i++){
    sprintf(hash.data() + (i * 2), "%02X", buff[i]);
  }

  cached = true;
  return hash;
}

iop::MacAddress & Device::macAddress() const noexcept {
  static iop::MacAddress mac;
  static bool cached = false;
  if (cached)
    return mac;

  struct ifreq ifr;
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);

  // TODO: error check
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	ioctl(fd, SIOCGIFHWADDR, &ifr);
	close(fd);
	
	unsigned char *bin = (unsigned char *) ifr.ifr_hwaddr.sa_data;
	
  for (uint8_t i = 0; i < 6; i++){
    sprintf(mac.data() + (i * 3), "%02X", bin[i]);
    if (i < 5) sprintf(mac.data() + (i * 3) + 2, ":");
  }
  cached = true;
  return mac;
}
}