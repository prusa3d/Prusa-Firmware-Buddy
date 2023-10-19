/// @file idler.cpp
#include "idler.h"
#include "buttons.h"
#include "globals.h"
#include "leds.h"
#include "motion.h"
#include "permanent_storage.h"
#include "../debug.h"

namespace modules {
namespace idler {

Idler idler;

void Idler::PrepareMoveToPlannedSlot() {
    mm::motion.PlanMoveTo<mm::Idler>(
        plannedMove == Operation::engage ? SlotPosition(plannedSlot) : IntermediateSlotPosition(plannedSlot),
        mm::unitToAxisUnit<mm::I_speed_t>(mg::globals.IdlerFeedrate_deg_s()));
    dbg_logic_fP(PSTR("Prepare Move Idler slot %d"), plannedSlot);
}

void Idler::PlanHomingMoveForward() {
    mm::motion.SetPosition(mm::Idler, mm::unitToSteps<mm::I_pos_t>(config::IdlerOffsetFromHome));
    axisStart = mm::axisUnitToTruncatedUnit<config::U_deg>(mm::motion.CurPosition<mm::Idler>());
    mm::motion.PlanMove<mm::Idler>(mm::unitToAxisUnit<mm::I_pos_t>(config::idlerLimits.lenght * 2),
        mm::unitToAxisUnit<mm::I_speed_t>(mg::globals.IdlerHomingFeedrate_deg_s()));
    dbg_logic_P(PSTR("Plan Homing Idler Forward"));
}

void Idler::PlanHomingMoveBack() {
    // we expect that we are at the front end of the axis, set the expected axis' position
    mm::motion.SetPosition(mm::Idler, mm::unitToSteps<mm::I_pos_t>(config::idlerLimits.lenght));
    axisStart = mm::axisUnitToTruncatedUnit<config::U_deg>(mm::motion.CurPosition<mm::Idler>());
    mm::motion.PlanMove<mm::Idler>(mm::unitToAxisUnit<mm::I_pos_t>(-config::idlerLimits.lenght * 2),
        mm::unitToAxisUnit<mm::I_speed_t>(mg::globals.IdlerHomingFeedrate_deg_s()));
    dbg_logic_P(PSTR("Plan Homing Idler Back"));
}

bool Idler::FinishHomingAndPlanMoveToParkPos() {
    // check the axis' length
    if (AxisDistance(mm::axisUnitToTruncatedUnit<config::U_deg>(mm::motion.CurPosition<mm::Idler>()))
        < (config::idlerLimits.lenght.v - 10)) { //@@TODO is 10 degrees ok?
        return false; // we couldn't home correctly, we cannot set the Idler's position
    }

    mm::motion.SetPosition(mm::Idler, 0);

    // finish whatever has been planned before homing
    if (plannedMove == Operation::disengage) {
        plannedSlot = IdleSlotIndex();
    }
    InitMovement();
    return true;
}

void Idler::FinishMove() {
    currentlyEngaged = plannedMove;
    hal::tmc2130::MotorCurrents c = mm::motion.CurrentsForAxis(mm::Idler);
    if (Disengaged()) // reduce power into the Idler motor when disengaged (less force necessary)
        SetCurrents(c.iRun, c.iHold);
    else if (Engaged()) { // maximum motor power when the idler is engaged
        SetCurrents(c.iRun, c.iRun);
    }
}

bool Idler::StallGuardAllowed(bool forward) const {
    const uint8_t checkDistance = forward ? 220 : 200;
    return AxisDistance(mm::axisUnitToTruncatedUnit<config::U_deg>(mm::motion.CurPosition<mm::Idler>())) > checkDistance;
}

Idler::OperationResult Idler::Disengage() {
    if (state == Moving || IsOnHold()) {
        dbg_logic_P(PSTR("Moving --> Disengage refused"));
        return OperationResult::Refused;
    }
    plannedSlot = IdleSlotIndex();
    plannedMove = Operation::disengage;

    // coordinates invalid, first home, then disengage
    if (!homingValid) {
        PlanHome();
        return OperationResult::Accepted;
    }

    // already disengaged
    if (Disengaged()) {
        dbg_logic_P(PSTR("Idler Disengaged"));
        return OperationResult::Accepted;
    }

    // disengaging
    return InitMovementNoReinitAxis();
}

Idler::OperationResult Idler::PartiallyDisengage(uint8_t slot) {
    return PlanMoveInner(slot, Operation::intermediate);
}

Idler::OperationResult Idler::Engage(uint8_t slot) {
    return PlanMoveInner(slot, Operation::engage);
}

Idler::OperationResult Idler::PlanMoveInner(uint8_t slot, Operation plannedOp) {
    if (state == Moving || IsOnHold()) {
        dbg_logic_P(PSTR("Moving --> Engage refused"));
        return OperationResult::Refused;
    }

    plannedSlot = slot;
    plannedMove = plannedOp;

    // if we are homing right now, just record the desired planned slot and return Accepted
    if (state == HomeBack) {
        return OperationResult::Accepted;
    }

    // coordinates invalid, first home, then engage
    // The MMU FW only decides to engage the Idler when it is supposed to do something and not while it is idle
    // so rebooting the MMU while the printer is printing (and thus holding the filament by the moving Idler)
    // should not be an issue
    if (!homingValid) {
        PlanHome();
        return OperationResult::Accepted;
    }

    // already engaged
    if (currentlyEngaged == plannedMove) {
        return OperationResult::Accepted;
    }

    return InitMovementNoReinitAxis();
}

bool Idler::Step() {
    if (IsOnHold()) {
        return true; // just wait, do nothing!
    }

    if (state != TMCFailed) {
        CheckTMC();
    }

    switch (state) {
    case Moving:
        // dbg_logic_P(PSTR("Moving Idler"));
        PerformMove();
        return false;
    case HomeForward:
        dbg_logic_P(PSTR("Homing Idler Forward"));
        PerformHomeForward();
        return false;
    case HomeBack:
        dbg_logic_P(PSTR("Homing Idler Back"));
        PerformHomeBack();
        return false;
    case Ready:
        if (!homingValid && mg::globals.FilamentLoaded() < mg::InFSensor) {
            PlanHome();
            return false;
        }
        return true;
    case TMCFailed:
    default:
        return true;
    }
}

void Idler::Init() {
    if (mg::globals.FilamentLoaded() < mg::InFSensor) {
        // home the Idler only in case we don't have filament loaded in the printer (or at least we think we don't)
        PlanHome();
    } else {
        // otherwise assume the Idler is at its idle position (that's where it usually is)
        mm::motion.SetPosition(mm::Idler, SlotPosition(IdleSlotIndex()).v);
        InvalidateHoming(); // and plan homing sequence ASAP
    }
}

} // namespace idler
} // namespace modules
