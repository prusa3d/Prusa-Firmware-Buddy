#include "motion.h"
#include "idler.h"
#include "stub_motion.h"

namespace modules {
namespace motion {

Motion motion;

// Intentionally inited with strange values
// Need to call ReinitMotion() each time we start some unit test
AxisSim axes[3] = {
    { -32767, false, false, false, config::pulley.sg_thrs, {} }, // pulley
    { -32767, false, false, false, config::selector.sg_thrs, {} }, // selector //@@TODO proper selector positions once defined
    { -32767, false, false, false, config::idler.sg_thrs, {} }, // idler
};

bool Motion::InitAxis(Axis axis) {
    SetEnabled(axis, true);
    return true;
}

bool Motion::InitAxis(Axis axis, MotorCurrents /*mc*/) {
    SetEnabled(axis, true);
    return true;
}

void Motion::SetEnabled(Axis axis, bool enabled) {
    axisData[axis].enabled = axes[axis].enabled = enabled;
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

void Motion::PlanStallGuardThreshold(Axis axis, int8_t sg_thrs) {
    // do nothing for now
    axes[axis].sg_thrs = sg_thrs;
}

void Motion::PlanMoveTo(Axis axis, pos_t pos, steps_t feed_rate, steps_t end_rate) {
    axes[axis].plannedMoves.push_back(pos);
    if (!axisData[axis].enabled)
        SetEnabled(axis, true);
}

pos_t Motion::Position(Axis axis) const {
    return axes[axis].pos;
}

pos_t Motion::CurPosition(Axis axis) const {
    return axes[axis].pos;
}

void Motion::SetPosition(Axis axis, pos_t x) {
    axes[axis].pos = x;
    axisData[axis].ctrl.SetPosition(axes[axis].pos);
}

void Motion::SetMode(Axis axis, hal::tmc2130::MotorMode mode) {
}

void Motion::SetMode(MotorMode mode) {
}

st_timer_t Motion::Step() {
    for (uint8_t i = 0; i < 3; ++i) {
        if (!axes[i].plannedMoves.empty()) {
            pos_t axisTargetPos = axes[i].plannedMoves.front();
            if (axes[i].pos != axisTargetPos) {
                int8_t dirInc = (axes[i].pos < axisTargetPos) ? 1 : -1;
                axes[i].pos += dirInc;
                axisData[i].ctrl.SetPosition(axes[i].pos);
            } else if (!axes[i].plannedMoves.empty()) {
                axes[i].plannedMoves.pop_front(); // one move completed, plan the next one
            }
        }
    }
    return 0;
}

bool Motion::QueueEmpty() const {
    for (uint8_t i = 0; i < 3; ++i) {
        if (!axes[i].plannedMoves.empty())
            return false;
    }
    return true;
}

bool Motion::QueueEmpty(Axis axis) const {
    return axes[axis].plannedMoves.empty();
}

uint8_t Motion::PlannedMoves(Axis axis) const {
    return axes[axis].plannedMoves.size();
}

void Motion::AbortPlannedMoves(bool halt) {
    for (uint8_t i = 0; i < 3; ++i) {
        AbortPlannedMoves((config::Axis)i, halt); // leave the axis where it was at the time of abort
    }
}

void Motion::AbortPlannedMoves(config::Axis i, bool) {
    axes[i].plannedMoves.clear(); // leave the axis where it was at the time of abort
    axisData[i].ctrl.SetPosition(axes[i].pos);
}

void ReinitMotion() {
    // reset the simulation data to defaults
    axes[0] = AxisSim({ 0, false, false, false, {} }); // pulley
    axes[1] = AxisSim({ unitToSteps<S_pos_t>(config::selectorSlotPositions[0]),
        false, false, false, {} }); // selector
    axes[2] = AxisSim({ unitToSteps<I_pos_t>(config::idlerSlotPositions[mi::Idler::IdleSlotIndex()]),
        false, false, false, {} }); // idler
}

bool PulleyEnabled() {
    return axes[0].enabled;
}

pos_t AxisNearestTargetPos(Axis axis) {
    if (axes[axis].plannedMoves.empty()) {
        return axes[axis].pos;
    } else {
        return axes[axis].plannedMoves.front();
    }
}

/// probably higher-level operations knowing the semantic meaning of axes

} // namespace motion
} // namespace modules
