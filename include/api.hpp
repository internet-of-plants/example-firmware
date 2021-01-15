#ifndef IOP_API_HPP
#define IOP_API_HPP

#include "fixed_string.hpp"
#include "log.hpp"
#include "models.hpp"
#include "network.hpp"
#include "option.hpp"
#include "result.hpp"
#include "static_string.hpp"
#include "storage.hpp"
#include "string_view.hpp"

#include "ArduinoJson.h"

/// Abstracts Internet of Plants API to avoid mistakes and properly report
/// errors
class Api {
private:
  Log logger;
  Network network_;

public:
  ~Api() = default;
  Api(const StaticString &host, const LogLevel logLevel) noexcept
      : logger(logLevel, F("API")), network_(host, logLevel) {}
  Api(Api const &other) = delete;
  Api(Api &&other) = delete;
  auto operator=(Api const &other) -> Api & = delete;
  auto operator=(Api &&other) -> Api & = delete;

  auto setup() const noexcept -> void { this->network().setup(); }

  static auto isConnected() noexcept -> bool { return Network::isConnected(); }
  static auto macAddress() noexcept -> String { return Network::macAddress(); }
  static auto disconnect() noexcept -> void { Network::disconnect(); }
  auto upgrade(const AuthToken &token, const MD5Hash &sketchHash) const noexcept
      -> ApiStatus;
  auto reportPanic(const AuthToken &authToken, const Option<PlantId> &id,
                   const PanicData &event) const noexcept -> ApiStatus;
  auto registerEvent(const AuthToken &token, const Event &event) const noexcept
      -> ApiStatus;
  auto registerPlant(const AuthToken &token) const noexcept
      -> Result<PlantId, ApiStatus>;
  auto authenticate(StringView username, StringView password) const noexcept
      -> Result<AuthToken, ApiStatus>;
  auto reportError(const AuthToken &authToken, const PlantId &id,
                   StringView error) const noexcept -> ApiStatus;
  auto registerLog(const AuthToken &authToken, const Option<PlantId> &plantId,
                   StringView log) const noexcept -> ApiStatus;
  auto host() const noexcept -> StaticString { return this->network().host(); };
  auto loggerLevel() const noexcept -> LogLevel;

  auto network() const noexcept -> const Network & { return this->network_; }

private:
  using JsonCallback = std::function<void(JsonDocument &)>;
  template <uint16_t SIZE>
  auto makeJson(const StaticString name, const JsonCallback func) const noexcept
      -> Option<FixedString<SIZE>> {
    auto doc = try_make_unique<StaticJsonDocument<SIZE>>();
    if (!doc) {
      this->logger.error(F("Unable to allocate at Api::makeJson for "), name);
      return Option<FixedString<SIZE>>();
    }
    func(*doc);

    if (doc->overflowed()) {
      const auto s = std::to_string(SIZE);
      this->logger.error(F("Payload doesn't fit Json<"), s, F("> at "), name);
      return Option<FixedString<SIZE>>();
    }

    auto buffer = FixedString<SIZE>::empty();
    serializeJson(*doc, buffer.asMut(), buffer.size);
    this->logger.debug(F("Json: "), *buffer);
    return Option<FixedString<SIZE>>(buffer);
  }
};

#include "utils.hpp"
#ifdef IOP_MONITOR
#undef IOP_MOCK_MONITOR
#endif
#ifndef IOP_MOCK_MONITOR
#ifndef IOP_MONITOR
#define IOP_API_DISABLED
#endif
#endif

#endif