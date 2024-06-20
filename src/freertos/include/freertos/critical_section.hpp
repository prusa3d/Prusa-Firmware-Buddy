#pragma once

namespace freertos {

/**
 * To be used for:
 * - Implementing atomic operations
 * on data shared between tasks and RTOS aware
 * interrupts.
 * - Precisely timed operations, which can
 * be interrupted for less than 10 microseconds
 * by high priority non OS aware interrupt.
 *
 * For atomic operations on stepper interrupt
 * data or timing with nanosecond precision use
 * DisableInterrupt instead.
 */
class [[nodiscard]] CriticalSection {
public:
    CriticalSection();
    ~CriticalSection();
};

} // namespace freertos
