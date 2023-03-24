/// @file unload_to_finda.cpp
#include "unload_to_finda.h"
#include "../modules/finda.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/permanent_storage.h"

namespace logic {

void UnloadToFinda::Reset(uint8_t maxTries) {
    this->maxTries = maxTries;
    // check the inital state of FINDA and plan the moves
    if (!mf::finda.Pressed()) {
        state = OK; // FINDA is already off, we assume the fillament is not there, i.e. already unloaded
    } else {
        // FINDA is sensing the filament, plan moves to unload it
        state = EngagingIdler;
        mi::idler.Engage(mg::globals.ActiveSlot());
    }
}

bool UnloadToFinda::Step() {
    switch (state) {
    case EngagingIdler:
        if (mg::globals.FilamentLoaded() >= mg::FilamentLoadState::InSelector) {
            state = UnloadingToFinda;
            mm::motion.InitAxis(mm::Pulley);
            ml::leds.SetMode(mg::globals.ActiveSlot(), ml::green, ml::blink0);
        } else {
            state = Failed;
        }
        return false;
    case UnloadingToFinda:
        if (mi::idler.Engaged()) {
            state = WaitingForFINDA;
            mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::InSelector);
            mm::motion.PlanMove<mm::Pulley>(-config::defaultBowdenLength - config::feedToFinda - config::filamentMinLoadedToMMU, config::pulleyFeedrate);
        }
        return false;
    case WaitingForFINDA:
        if (!mf::finda.Pressed()) {
            // detected end of filament
            state = OK;
            mm::motion.AbortPlannedMoves(); // stop rotating the pulley
            ml::leds.SetMode(mg::globals.ActiveSlot(), ml::green, ml::off);
        } else if (/*tmc2130_read_gstat() &&*/ mm::motion.QueueEmpty()) {
            // we reached the end of move queue, but the FINDA didn't switch off
            // two possible causes - grinded filament or malfunctioning FINDA
            if (--maxTries) {
                Reset(maxTries); // try again
            } else {
                state = Failed;
            }
        }
        return false;
    case OK:
    case Failed:
    default:
        return true;
    }
}

} // namespace logic
