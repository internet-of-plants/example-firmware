#ifndef IOP_API_HPP
#define IOP_API_HPP

#include "certificate_storage.hpp"

#include "fixed_string.hpp"
#include "log.hpp"
#include "models.hpp"
#include "network.hpp"
#include "option.hpp"
#include "result.hpp"
#include "static_string.hpp"
#include "storage.hpp"
#include "string_view.hpp"
#include "tracer.hpp"

#include "ArduinoJson.h"

/// Abstracts Internet of Plants API to avoid mistakes and properly report
/// errors
class Api {
private:
  Log logger;
  Network network_;

public:
  ~Api();
  Api(StaticString uri, LogLevel logLevel) noexcept;

  Api(Api const &other);
  Api(Api &&other) = delete;
  auto operator=(Api const &other) -> Api &;
  auto operator=(Api &&other) -> Api & = delete;

  auto setup() const noexcept -> void;

  auto upgrade(const AuthToken &token, const MacAddress &mac,
               const MD5Hash &sketchHash) const noexcept -> ApiStatus;
  auto reportPanic(const AuthToken &authToken, const MacAddress &mac,
                   const PanicData &event) const noexcept -> ApiStatus;
  auto registerEvent(const AuthToken &token, const Event &event) const noexcept
      -> ApiStatus;
  auto authenticate(StringView username, StringView password,
                    const MacAddress &mac) const noexcept
      -> Result<AuthToken, ApiStatus>;
  auto registerLog(const AuthToken &authToken, const MacAddress &mac,
                   StringView log) const noexcept -> ApiStatus;

  auto uri() const noexcept -> StaticString;
  auto loggerLevel() const noexcept -> LogLevel;
  auto network() const noexcept -> const Network &;

  // private:
  using JsonCallback = std::function<void(JsonDocument &)>;
  template <uint16_t SIZE>
  auto makeJson(const StaticString name, const JsonCallback func) const noexcept
      -> Option<FixedString<SIZE>> {
    IOP_TRACE();
    auto doc = try_make_unique<StaticJsonDocument<SIZE>>();
    if (!doc) {
      this->logger.error(F("Unable to allocate "), String(SIZE),
                         F(" bytes at Api::makeJson for "), name);
      return Option<FixedString<SIZE>>();
    }
    func(*doc);

    if (doc->overflowed()) {
      const auto s = std::to_string(SIZE);
      this->logger.error(F("Payload doesn't fit Json<"), s, F("> at "), name);
      return Option<FixedString<SIZE>>();
    }

    auto fixed = FixedString<SIZE>::empty();
    serializeJson(*doc, fixed.asMut(), fixed.size);
    this->logger.debug(F("Json: "), *fixed);
    return Option<FixedString<SIZE>>(fixed);
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
#else
#ifndef IOP_MONITOR
#define IOP_API_DISABLED
#endif
#endif

#endif