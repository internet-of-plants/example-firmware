#include "driver/string.hpp"

namespace driver {
    void setup() noexcept;
    void loop() noexcept;
    #ifdef IOP_POSIX
    std::string_view execution_path() noexcept;
    uintmax_t stack_size() noexcept;
    #endif
}