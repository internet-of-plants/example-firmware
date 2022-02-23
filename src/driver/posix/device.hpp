#include "driver/device.hpp"
#include "driver/runtime.hpp"
#include "driver/panic.hpp"

#include "driver/cpp17/device.hpp"

#include <stdint.h>

// POSIX
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>

namespace driver {
// NOOP for now as we assume the host will do it for us, eventually we will want to do it by ourselves
void Device::syncNTP() const noexcept {}

auto Device::vcc() const noexcept -> uint16_t { return UINT16_MAX; }

auto Device::platform() const noexcept -> iop::StaticString { return IOP_STR("POSIX"); }

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