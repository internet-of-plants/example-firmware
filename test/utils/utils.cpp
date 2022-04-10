#include "utils.hpp"

#include <unity.h>

void interrupts() {
    TEST_ASSERT(iop::descheduleInterrupt() == InterruptEvent::NONE);
    iop::scheduleInterrupt(InterruptEvent::MUST_UPGRADE);
    TEST_ASSERT(iop::descheduleInterrupt() == InterruptEvent::MUST_UPGRADE);
    TEST_ASSERT(iop::descheduleInterrupt() == InterruptEvent::NONE);

    iop::scheduleInterrupt(InterruptEvent::MUST_UPGRADE);
    iop::scheduleInterrupt(InterruptEvent::MUST_UPGRADE);
    iop::scheduleInterrupt(InterruptEvent::ON_CONNECTION);
    TEST_ASSERT(iop::descheduleInterrupt() == InterruptEvent::MUST_UPGRADE);
    TEST_ASSERT(iop::descheduleInterrupt() == InterruptEvent::ON_CONNECTION);
    TEST_ASSERT(iop::descheduleInterrupt() == InterruptEvent::NONE);
    TEST_ASSERT(iop::descheduleInterrupt() == InterruptEvent::NONE);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(interrupts);
    UNITY_END();
    return 0;
}