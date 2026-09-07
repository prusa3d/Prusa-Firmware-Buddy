/// @file
#pragma once

#include <core/millis_t.h>
#include <cstdint>

class ThermalRunaway {

public:
    /// Manages the thermal runaway protection
    /// @returns true while a runaway is detected; the caller is expected to raise the error
    [[nodiscard]] bool step(float current, float target, uint16_t period_seconds, uint16_t hysteresis_degc);

    /// Re-arm from scratch. Call when heater control is (re)entered after being inactive
    /// (e.g. INDX tool pickup), otherwise stale timer/state can trip immediately.
    void reset(float target = 0.0f);

private:
    enum State : uint8_t {
        TRInactive,
        TRFirstHeating,
        TRStable,
        TRRunaway,
    };

private:
    millis_t timer = 0;
    State state = TRInactive;
    float tr_target_temperature = 0.0;
};
