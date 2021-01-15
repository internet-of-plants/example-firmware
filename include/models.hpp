#ifndef IOP_MODELS_HPP
#define IOP_MODELS_HPP

#include <array>
#include <cstdint>
#include <memory>

#include "result.hpp"
#include "storage.hpp"
#include "string_view.hpp"

// Those are basically utils::Storage but typesafe
TYPED_STORAGE(AuthToken, 64);
TYPED_STORAGE(PlantId, 19);
TYPED_STORAGE(NetworkName, 32);
TYPED_STORAGE(NetworkPassword, 64);
TYPED_STORAGE(MD5Hash, 32);

auto hashSketch() noexcept -> MD5Hash;

typedef uint16_t HttpCode;

using PanicData = struct panic_data_ {
  // TODO: this could have a StaticString alternative to be able to use `_P`
  // PROGMEM methods
  StringView msg;
  StaticString file;
  uint32_t line;
  StringView func;
};

using WifiCredentials = struct wifiCredentials {
  NetworkName ssid;
  NetworkPassword password;
};

using EventStorage = struct eventStorage {
  float airTemperatureCelsius;
  float airHumidityPercentage;
  float airHeatIndexCelsius;
  uint16_t soilResistivityRaw;
  float soilTemperatureCelsius;
};

class Event {
public:
  EventStorage storage; // NOLINT misc-non-private-member-variables-in-classes
  PlantId plantId;      // NOLINT misc-non-private-member-variables-in-classes
  MD5Hash firmwareHash; // NOLINT misc-non-private-member-variables-in-classes
  ~Event() = default;
  Event(EventStorage storage, PlantId plantId, MD5Hash firmwareHash) noexcept
      : storage(storage), plantId(std::move(plantId)),
        firmwareHash(std::move(firmwareHash)) {}
  Event(Event const &ev) = default;
  Event(Event &&ev) = default;
  auto operator=(Event const &ev) -> Event & = default;
  auto operator=(Event &&ev) noexcept -> Event & = default;
};

#endif