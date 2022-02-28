#ifndef IOP_DRIVER_WIFI_CLIENT
#define IOP_DRIVER_WIFI_CLIENT

#include "ESP8266WiFi.h"

#ifdef IOP_SSL
using NetworkClient = BearSSL::WiFiClientSecure;
#else
using NetworkClient = WiFiClient;
#endif

#endif