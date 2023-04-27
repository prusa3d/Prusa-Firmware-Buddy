/// @file timebase.h
#pragma once
#include <stdint.h>

namespace modules {

/// The time namespace provides all necessary facilities related to measuring real elapsed time for the whole firmware.
namespace time {

/// A basic time tracking class
/// Works on top of processor timers and provides real-time steady clock
/// (at least what the CPU thinks ;) )
class Timebase {
public:
    constexpr inline Timebase()
        : ms(0) {}

    /// Initializes the Timebase class - sets the timers and prepares the internal variables.
    void Init();

    /// @returns current milliseconds elapsed from the initialization of this class
    ///  (usually the start of the firmware)
    uint16_t Millis() const;

    void Isr();

    /// @returns true if the timeout elapsed from the start
    /// handles correctly timer counter overflows
    bool Elapsed(uint16_t start, uint16_t timeout) const;

private:
    uint16_t ms;
};

/// The one and only instance of Selector in the FW
extern Timebase timebase;

} // namespace time
} // namespace modules

namespace mt = modules::time;
