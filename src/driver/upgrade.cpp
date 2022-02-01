#ifdef IOP_DESKTOP
#include "driver/desktop/upgrade.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/upgrade.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/upgrade.hpp"
#elif defined(IOP_ESP32)
#include "driver/esp32/upgrade.hpp"
#else
#error "Target not supported"
#endif