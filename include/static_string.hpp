#ifndef IOP_STATIC_STRING_HPP
#define IOP_STATIC_STRING_HPP

#include "WString.h"

class StringView;

/// Helper string that holds a pointer to a string stored in PROGMEM
/// It's here to provide a typesafe way to handle PROGMEM data and to avoid
/// defaulting to String(__FlashStringHelper*) constructor and allocating
/// implicitly
///
/// In ESP8266 the PROGMEM can just be accessed like any other RAM (besides the
/// correct alignment) So we technically don't need it to be type safe. But it's
/// a safe storage for it. And it works very well with `StringView` because of
/// that
class StaticString {
private:
  const __FlashStringHelper *str;

public:
  ~StaticString() = default;

  // NOLINTNEXTLINE hicpp-explicit-conversions
  constexpr StaticString(const __FlashStringHelper *str) noexcept : str(str) {}
  constexpr StaticString(StaticString const &other) noexcept = default;
  constexpr StaticString(StaticString &&other) noexcept : str(other.str) {}
  auto operator=(StaticString const &other) noexcept
      -> StaticString & = default;
  auto operator=(StaticString &&other) noexcept -> StaticString & = default;
  auto get() const noexcept -> const __FlashStringHelper * { return this->str; }
  auto contains(StaticString needle) const noexcept -> bool;
  auto contains(StringView needle) const noexcept -> bool;
  auto length() const noexcept -> size_t { return strlen_P(this->asCharPtr()); }
  auto isEmpty() const noexcept -> bool { return this->length() == 0; }
  auto asCharPtr() const noexcept -> const char * {
    return reinterpret_cast<const char *const>(this->get());
  }
};

#define PROGMEM_STRING(name, msg)                                              \
  static const char *const PROGMEM name##_progmem_char = msg;                  \
  static const StaticString name(                                              \
      reinterpret_cast<const __FlashStringHelper *>(name##_progmem_char));

#endif