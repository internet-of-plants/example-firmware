#include <unity.h>

#include "iop/runtime.hpp"

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    UNITY_BEGIN();
    driver::setup();
    UNITY_END();
    return 0;
}