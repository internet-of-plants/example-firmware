#ifdef IOP_DESKTOP
#include "driver/desktop/interrupt.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/interrupt.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/interrupt.hpp"
#elif defined(IOP_ESP32)
#include "driver/esp32/interrupt.hpp"
#else
#error "Target not supported"
#endif