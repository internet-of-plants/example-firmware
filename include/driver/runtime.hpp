#include "driver/string.hpp"

namespace driver {
    /// Should be defined by the application
    ///
    /// Initializes system, run only once
    void setup() noexcept;

    /// Should be defined by the application
    ///
    /// Event loop function, continuously scheduled for execution
    void loop() noexcept;

    // FIXME: find a better place for these definitions
    #ifdef IOP_POSIX
    /// Horrible hack to help accessing argv[0], as it contains the full path to the current binary
    /// Not exclusive to POSIX
    auto execution_path() noexcept -> std::string_view;

    /// Horrible hack to help calculating used stack, it gets the stack pointer from the beggining of main
    /// And compares with current stack pointer.
    auto stack_used() noexcept -> uintmax_t;
    #endif
}