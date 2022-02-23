#include <unity.h>

#include "driver/runtime.hpp"

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    UNITY_BEGIN();
    driver::setup();
    UNITY_END();
    return 0;
}