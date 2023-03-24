/// @file feed_to_bondtech.cpp
#include "feed_to_bondtech.h"
#include "../modules/buttons.h"
#include "../modules/fsensor.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/permanent_storage.h"
#include "../debug.h"

namespace logic {

void FeedToBondtech::Reset(uint8_t maxRetries) {
    dbg_logic_P(PSTR("\nFeed to Bondtech\n\n"));
    state = EngagingIdler;
    this->maxRetries = maxRetries;
    ml::leds.SetMode(mg::globals.ActiveSlot(), ml::green, ml::blink0);
    mi::idler.Engage(mg::globals.ActiveSlot());
}

void logic::FeedToBondtech::GoToPushToNozzle() {
    mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::InFSensor);
    // plan a slow move to help push filament into the nozzle
    //@@TODO the speed in mm/s must correspond to printer's feeding speed!
    mm::motion.PlanMove<mm::Pulley>(config::fsensorToNozzle, config::pulleySlowFeedrate);
    state = PushingFilamentIntoNozzle;
}

bool FeedToBondtech::Step() {
    switch (state) {
    case EngagingIdler:
        if (mi::idler.Engaged()) {
            dbg_logic_P(PSTR("Feed to Bondtech --> Idler engaged"));
            dbg_logic_fP(PSTR("Pulley start steps %u"), mm::motion.CurPosition(mm::Pulley));
            state = PushingFilamentToFSensor;
            mm::motion.InitAxis(mm::Pulley);
            mm::motion.PlanMove<mm::Pulley>(config::defaultBowdenLength, config::pulleyFeedrate, config::pulleySlowFeedrate);
        }
        return false;
    case PushingFilamentToFSensor:
        //dbg_logic_P(PSTR("Feed to Bondtech --> Pushing"));
        if (mfs::fsensor.Pressed()) {
            mm::motion.AbortPlannedMoves(); // stop pushing filament
            GoToPushToNozzle();
        } else if (mm::motion.StallGuard(mm::Pulley)) {
            // stall guard occurred during movement - the filament got stuck
            state = Failed; // @@TODO may be even report why it failed
        } else if (mm::motion.QueueEmpty()) { // all moves have been finished and the fsensor didn't switch on
            state = Failed;
        }
        return false;
    case PushingFilamentIntoNozzle:
        if (mm::motion.QueueEmpty()) {
            mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::InNozzle);
            mi::idler.Disengage();
            // while disengaging the idler, keep on moving with the pulley to avoid grinding while the printer is trying to grab the filament itself
            mm::motion.PlanMove<mm::Pulley>(config::fsensorToNozzleAvoidGrind, config::pulleySlowFeedrate);
            state = DisengagingIdler;
        }
        return false;
    case DisengagingIdler:
        if (!mi::idler.Engaged()) {
            dbg_logic_P(PSTR("Feed to Bondtech --> Idler disengaged"));
            dbg_logic_fP(PSTR("Pulley end steps %u"), mm::motion.CurPosition(mm::Pulley));
            state = OK;
            mm::motion.Disable(mm::Pulley);
            ml::leds.SetMode(mg::globals.ActiveSlot(), ml::green, ml::on);
        }
        return false;
    case OK:
        dbg_logic_P(PSTR("Feed to Bondtech OK"));
        return true;
    case Failed:
        dbg_logic_P(PSTR("Feed to Bondtech FAILED"));
        return true;
    default:
        return true;
    }
}

} // namespace logic
