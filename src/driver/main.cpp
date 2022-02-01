#ifdef IOP_DESKTOP
#ifndef UNIT_TEST
#include "driver/desktop/main.hpp"
#endif
#elif defined(IOP_ESP8266)
#include "driver/esp8266/main.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/main.hpp"
#elif defined(IOP_ESP32)
#include "driver/esp32/main.hpp"
#else
#error "Target not supported"
#endif