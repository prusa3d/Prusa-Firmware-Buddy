/// @file
#pragma once

#include <stdint.h>
#include "../config/config.h"

/// The modules namespace contains models of MMU's components
namespace modules {
namespace voltage {

/// We are measuring the bandgap voltage, Vb=1.1V
/// To compute the threshold value: `VAL = 1125.3 / AVCC`
/// So for:
/// - AVCC=5V, you get VAL=225.06
/// - AVCC=4.1V, you get VAL=274.46
/// - AVCC=4V, you get VAL=281.35
///
/// Any lower and the board will probably die sooner than being able to report anything.
/// The TMC will die, but the atmega32u4 seems to be a bit more resilient.
/// It's still out of spec anyway since the atmega32u4 is rated for 16MHz only at >4.5V.
class VCC {
public:
    inline constexpr VCC()
        : vcc_val(0) {}

    /// Reads the ADC, checks the value
    void Step();

    /// @returns the current bandgap voltage level, platform dependent.
    /// @note see VCC measurement setup in config.h
    uint16_t CurrentBandgapVoltage() const { return vcc_val; }

private:
    uint16_t vcc_val;
};

/// The one and only instance of Undervoltage_check in the FW
extern VCC vcc;

} // namespace voltage
} // namespace modules

namespace mv = modules::voltage;
