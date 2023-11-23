/// @file pulley.cpp
#include "pulley.h"
#include "buttons.h"
#include "globals.h"
#include "leds.h"
#include "permanent_storage.h"
#include "../debug.h"

namespace modules {
namespace pulley {

Pulley pulley;

bool __attribute__((noinline)) Pulley::FinishHomingAndPlanMoveToParkPos() {
    mm::motion.SetPosition(mm::Pulley, 0);
    return true;
}

bool Pulley::Step() {
    if (IsOnHold()) {
        return true; // just wait, do nothing!
    }
    if (state != TMCFailed) {
        CheckTMC();
    }

    switch (state) {
    case Moving:
        PerformMove();
        return false;
    case HomeBack:
        homingValid = true;
        FinishHomingAndPlanMoveToParkPos();
        return true;
    case Ready:
        return true;
    case TMCFailed:
    default:
        return true;
    }
}

void Pulley::PlanMove(mm::P_pos_t delta, mm::P_speed_t feed_rate, mm::P_speed_t end_rate) {
    mm::motion.PlanMove<mm::Pulley>(delta, feed_rate, end_rate);
    state = Moving;
}

int32_t Pulley::CurrentPosition_mm() {
    return mm::axisUnitToTruncatedUnit<config::U_mm>(mm::motion.CurPosition<mm::Pulley>());
}

void __attribute__((noinline)) Pulley::InitAxis() {
    mm::motion.InitAxis(mm::Pulley);
}

void Pulley::Disable() {
    mm::motion.Disable(mm::Pulley);
    state = Ready;
}

} // namespace pulley
} // namespace modules
