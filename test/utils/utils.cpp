#include "utils.hpp"

#include <unity.h>

void interrupts() {
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::NONE);
    utils::scheduleInterrupt(InterruptEvent::FACTORY_RESET);
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::FACTORY_RESET);
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::NONE);

    utils::scheduleInterrupt(InterruptEvent::FACTORY_RESET);
    utils::scheduleInterrupt(InterruptEvent::FACTORY_RESET);
    utils::scheduleInterrupt(InterruptEvent::MUST_UPGRADE);
    utils::scheduleInterrupt(InterruptEvent::ON_CONNECTION);
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::FACTORY_RESET);
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::MUST_UPGRADE);
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::ON_CONNECTION);
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::NONE);
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::NONE);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(interrupts);
    UNITY_END();
    return 0;
}