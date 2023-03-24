#include "tmc2130.h"

namespace hal {
namespace tmc2130 {

void TMC2130::SetMode(const MotorParams &params, MotorMode mode) {
    // TODO
}

bool TMC2130::Init(const MotorParams &params,
    const MotorCurrents &currents,
    MotorMode mode) {
    // TODO
    return true;
}

void TMC2130::SetEnabled(const MotorParams &params, bool enabled) {
    this->enabled = enabled;
}

void TMC2130::Isr(const MotorParams &params) {
}

} // namespace tmc2130
} // namespace hal
