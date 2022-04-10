#include <unity.h>

#include "iop-hal/runtime.hpp"

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    UNITY_BEGIN();
    iop_hal::setup();
    UNITY_END();
    return 0;
}