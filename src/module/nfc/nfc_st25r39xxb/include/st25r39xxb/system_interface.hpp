#pragma once

#include <cstdint>

namespace st25r39xxb {

class SystemInterface {
public:
    virtual void delay(uint32_t delay_ms) = 0;

    /// Blocks until \ref trigger_interrupt is called or \p timeout_ms elapses, and returns how much
    /// of the timeout is left. A wakeup left over from an earlier assertion may return immediately -
    /// this is a hint that the chip is worth looking at, not a promise that it is.
    virtual uint32_t await_interrupt(uint32_t timeout_ms) = 0;

    /// Called from the platform's IRQ handler. Must tolerate being called several times before a
    /// single \ref await_interrupt consumes the result - the driver's own status reads and mask
    /// writes move the chip's IRQ line, so assertions and waits do not pair up one to one.
    virtual void trigger_interrupt() = 0;
};

} // namespace st25r39xxb
