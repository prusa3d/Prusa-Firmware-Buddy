/// @file movable_base.cpp
#include "movable_base.h"
#include "globals.h"
#include "motion.h"

namespace modules {
namespace motion {

MovableBase::OperationResult MovableBase::PlanHome() {
    InvalidateHoming();
    if (IsOnHold())
        return OperationResult::Refused;

    // switch to normal mode on this axis

    // Update StallGuard threshold only when planning a homing move
    mm::motion.PlanStallGuardThreshold(axis, mg::globals.StallGuardThreshold(axis));
    mm::motion.InitAxis(axis);
    mm::motion.SetMode(axis, mm::Normal);
    mm::motion.StallGuardReset(axis);

    // plan move at least as long as the axis can go from one side to the other
    state = HomeForward; // beware - the derived class may change the state if necessary
    currentSlot = -1; // important - other state machines may be waiting for a valid Slot() which is not yet correct while homing in progress
    PlanHomingMoveForward();
    return OperationResult::Accepted;
}

void __attribute__((noinline)) MovableBase::SetCurrents(uint8_t iRun, uint8_t iHold) {
    hal::tmc2130::MotorCurrents tempCurrent(iRun, iHold);
    mm::motion.DriverForAxis(axis).SetCurrents(mm::axisParams[axis].params, tempCurrent);
}

void MovableBase::HoldOn() {
    state |= OnHold; // set the on-hold bit
    mm::motion.AbortPlannedMoves(axis);
    // Force turn off motors - prevent overheating and allow servicing during an error state.
    // And don't worry about TMC2130 creep after axis enabled - we'll rehome both axes later when needed.
    mm::motion.Disable(axis);
}

MovableBase::OperationResult MovableBase::InitMovement() {
    if (motion.InitAxis(axis)) {
        return InitMovementNoReinitAxis();
    } else {
        state = TMCFailed;
        return OperationResult::Failed;
    }
}

MovableBase::OperationResult __attribute__((noinline)) MovableBase::InitMovementNoReinitAxis() {
    hal::tmc2130::MotorCurrents c = mm::motion.CurrentsForAxis(axis);
    SetCurrents(c.iRun, c.iHold);
    PrepareMoveToPlannedSlot();
    state = Moving;
    return OperationResult::Accepted;
}

void MovableBase::PerformMove() {
    if (mm::motion.QueueEmpty(axis)) {
        // move finished
        currentSlot = plannedSlot;
        FinishMove();
        state = Ready;
    }
}

void MovableBase::PerformHomeForward() {
    if (mm::motion.StallGuard(axis)) {
        // we have reached the front end of the axis - first part homed probably ok
        mm::motion.StallGuardReset(axis);
        if (StallGuardAllowed(true)) {
            mm::motion.AbortPlannedMoves(axis, true);
            PlanHomingMoveBack();
            state = HomeBack;
        }
    } else if (mm::motion.QueueEmpty(axis)) {
        HomeFailed();
    }
}

void MovableBase::PerformHomeBack() {
    if (mm::motion.StallGuard(axis)) {
        // we have reached the back end of the axis - second part homed probably ok
        mm::motion.StallGuardReset(axis);
        if (StallGuardAllowed(false)) {
            mm::motion.AbortPlannedMoves(axis, true);
            mm::motion.SetMode(axis, mg::globals.MotorsStealth() ? mm::Stealth : mm::Normal);
            if (!FinishHomingAndPlanMoveToParkPos()) {
                // the measured axis' length was incorrect, something is blocking it, report an error, homing procedure terminated
                HomeFailed();
            } else {
                homingValid = true;
                // state = Ready; // not yet - we have to move to our parking or target position after homing the axis
            }
        }
    } else if (mm::motion.QueueEmpty(axis)) {
        HomeFailed();
    }
}

void MovableBase::HomeFailed() {
    // we ran out of planned moves but no StallGuard event has occurred
    // or the measured length of axis was not within the accepted tolerance
    homingValid = false;
    mm::motion.Disable(axis); // disable power to the axis - allows the user to do something with the device manually
    state = HomingFailed;
}

void MovableBase::CheckTMC() {
    if (mm::motion.DriverForAxis(axis).CheckForErrors(axisParams[axis].params)) {
        // TMC2130 entered some error state, the planned move couldn't have been finished - result of operation is Failed
        tmcErrorFlags = mm::motion.DriverForAxis(axis).GetErrorFlags(); // save the failed state
        mm::motion.AbortPlannedMoves(axis, true);
        state = TMCFailed;
    }
}

uint16_t __attribute__((noinline)) MovableBase::AxisDistance(int32_t curPos) const {
    return abs(curPos - axisStart);
}

} // namespace motion
} // namespace modules
