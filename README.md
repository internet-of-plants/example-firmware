# Example Internet-of-Plants Firmware

For ESP8266 NodeMCUv2.

If credentials aren't available, opens captive portal to obtain the WiFi credentials and the internet-of-plants server credentials.

The Access Point created for the Captive Portal will have "iop" as SSID and a PSK generated during the first local compilation, stored in `include/generated/psk.hpp`.

After connecting to the WiFi and authenticating to the internet-of-plants server, it will constantly send measurements to the server and fetch for updates from it.

Reports `air_temperature_celsius` and `air_humidity_percentage` from `Pin::D6`, `soil_resistivity_raw` from `Analog0` with `Pin::D7` as power supply, `soil_temperature_celsius` from `Pin::D5` to the server every 5 minutes.

Monitors `Pin::D1` for button presses, working as factory reset trigger (deletes everything stored in the persistent memory).

## License

[GNU Affero General Public License version 3 or later (AGPL-3.0+)](https://github.com/internet-of-plants/default-firmware/blob/main/LICENSE.md)