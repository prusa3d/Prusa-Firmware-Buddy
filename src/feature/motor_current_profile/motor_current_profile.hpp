#pragma once

#include <cstdint>

namespace buddy {

/// Motor current values in mA for each axis
struct MotorCurrentProfile {
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint16_t e;
};

enum class StandardMotorCurrentProfile : uint8_t {
    fw_default = 0, ///< Firmware default currents for all axes
    increased_e = 1, ///< Increased E stepper current for higher torque
    decreased_e_flex = 2, ///< Decreased E stepper current for flexible filaments
    _count,
};

const MotorCurrentProfile &standard_motor_current_profile(StandardMotorCurrentProfile profile);

StandardMotorCurrentProfile active_motor_current_profile();
void set_active_motor_current_profile(StandardMotorCurrentProfile profile);

} // namespace buddy
