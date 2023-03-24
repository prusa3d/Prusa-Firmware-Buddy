#include "motion.h"
#include "idler.h"
#include "stub_motion.h"

namespace modules {
namespace motion {

Motion motion;

// Intentionally inited with strange values
// Need to call ReinitMotion() each time we start some unit test
AxisSim axes[3] = {
    { -32767, -32767, false, false, false }, // pulley
    { -32767, -32767, false, false, false }, // selector //@@TODO proper selector positions once defined
    { -32767, -32767, false, false, false }, // idler
};

bool Motion::InitAxis(Axis axis) {
    axes[axis].enabled = true;
    return true;
}

void Motion::SetEnabled(Axis axis, bool enabled) {
    axes[axis].enabled = enabled;
}

bool Motion::StallGuard(Axis axis) {
    return axes[axis].stallGuard;
}

void Motion::StallGuardReset(Axis axis) {
    axes[axis].stallGuard = false;
}

void TriggerStallGuard(Axis axis) {
    axes[axis].stallGuard = true;
}

void Motion::PlanMoveTo(Axis axis, pos_t pos, steps_t feed_rate, steps_t end_rate) {
    axes[axis].targetPos = pos;
    if (!axisData[axis].enabled)
        SetEnabled(axis, true);
}

pos_t Motion::Position(Axis axis) const {
    return axes[axis].pos;
}

void Motion::SetPosition(Axis axis, pos_t x) {
    axes[axis].pos = x;
}

void Motion::SetMode(Axis axis, hal::tmc2130::MotorMode mode) {
}

st_timer_t Motion::Step() {
    for (uint8_t i = 0; i < 3; ++i) {
        if (axes[i].pos != axes[i].targetPos) {
            int8_t dirInc = (axes[i].pos < axes[i].targetPos) ? 1 : -1;
            axes[i].pos += dirInc;
        }
    }
    return 0;
}

bool Motion::QueueEmpty() const {
    for (uint8_t i = 0; i < 3; ++i) {
        if (axes[i].pos != axes[i].targetPos)
            return false;
    }
    return true;
}

bool Motion::QueueEmpty(Axis axis) const {
    return axes[axis].pos == axes[axis].targetPos;
}

void Motion::AbortPlannedMoves(bool halt) {
    for (uint8_t i = 0; i < 3; ++i) {
        AbortPlannedMoves((config::Axis)i, halt); // leave the axis where it was at the time of abort
    }
}

void Motion::AbortPlannedMoves(config::Axis i, bool) {
    axes[i].targetPos = axes[i].pos; // leave the axis where it was at the time of abort
}

void ReinitMotion() {
    // reset the simulation data to defaults
    axes[0] = AxisSim({ 0, 0, false, false, false }); // pulley
    axes[1] = AxisSim({ unitToSteps<S_pos_t>(config::selectorSlotPositions[0]),
        unitToSteps<S_pos_t>(config::selectorSlotPositions[0]),
        false, false, false }); // selector
    axes[2] = AxisSim({ unitToSteps<I_pos_t>(config::idlerSlotPositions[mi::Idler::IdleSlotIndex()]),
        unitToSteps<I_pos_t>(config::idlerSlotPositions[mi::Idler::IdleSlotIndex()]),
        false, false, false }); // idler
}

bool PulleyEnabled() {
    return axes[0].enabled;
}

/// probably higher-level operations knowing the semantic meaning of axes

} // namespace motion
} // namespace modules
