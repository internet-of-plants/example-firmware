#include "storage.hpp"

#include <unity.h>

void authToken() {
  const Storage storage(iop::LogLevel::WARN);
  storage.setup();
  storage.removeToken();
  TEST_ASSERT(!storage.token());
  storage.setToken(AuthToken::fromBytesUnsafe(reinterpret_cast<const uint8_t*>("Bruh"), 4));
  TEST_ASSERT(memcmp(storage.token().value().constPtr(), "Bruh", 4) == 0);
  storage.removeToken();
  TEST_ASSERT(!storage.token());
}

void wifiConfig() {
  const Storage storage(iop::LogLevel::WARN);
  storage.setup();
  storage.removeWifi();
  TEST_ASSERT(!storage.wifi());
  const auto bruh = reinterpret_cast<const uint8_t*>("Bruh");
  const WifiCredentials creds(NetworkName::fromBytesUnsafe(bruh, 4), NetworkPassword::fromBytesUnsafe(bruh, 4));
  storage.setWifi(creds);
  TEST_ASSERT(storage.wifi().value().ssid.asString().borrow() == creds.ssid.asString().borrow());
  TEST_ASSERT(storage.wifi().value().password.asString().borrow() == creds.password.asString().borrow());
  storage.removeWifi();
  TEST_ASSERT(!storage.wifi());
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(authToken);
    RUN_TEST(wifiConfig);
    UNITY_END();
    return 0;
}