#include "driver/device.hpp"

#include "driver/cpp17/md5.hpp"

#include <thread>
#include <filesystem>
#include <fstream>

namespace driver {
auto Device::availableStorage() const noexcept -> uintmax_t {
  // TODO: handle errors
  std::error_code code;
  auto available = std::filesystem::space(std::filesystem::current_path(), code).available;
  if (code) return 0;
  return available;
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
  iop_assert(file.is_open(), IOP_STR("Unable to open firmware file"));

  const auto data = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  iop_assert(!file.fail(), IOP_STR("Unable to read from firmware file"));

  file.close();
  iop_assert(!file.fail(), IOP_STR("Unable to close firmware file"));

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
}