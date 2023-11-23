/// @file feed_to_bondtech.cpp
#include "feed_to_bondtech.h"
#include "../modules/buttons.h"
#include "../modules/fsensor.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/permanent_storage.h"
#include "../modules/pulley.h"
#include "../debug.h"
#include "../config/axis.h"

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
    // the speed in mm/s must correspond to printer's feeding speed!
    mpu::pulley.PlanMove(mg::globals.FSensorToNozzle_mm(), mg::globals.PulleySlowFeedrate_mm_s());
    state = PushingFilamentIntoNozzle;
}

bool FeedToBondtech::Step() {
    switch (state) {
    case EngagingIdler:
        if (mi::idler.Engaged()) {
            dbg_logic_P(PSTR("Feed to Bondtech --> Idler engaged"));
            dbg_logic_fP(PSTR("Pulley start steps %u"), mpu::pulley.CurrentPosition_mm());
            state = PushingFilamentFast;
            mpu::pulley.InitAxis();
            // plan a fast move while in the safe minimal length
            // fast feed in millimeters - if the EEPROM value is incorrect, we'll get the default length
            unit::U_mm fastFeedDistance = { (long double)mps::BowdenLength::Get() };
            mpu::pulley.PlanMove(fastFeedDistance,
                mg::globals.PulleyLoadFeedrate_mm_s(),
                mg::globals.PulleySlowFeedrate_mm_s());
            // plan additional slow move while waiting for fsensor to trigger
            mpu::pulley.PlanMove(config::maximumBowdenLength - fastFeedDistance,
                mg::globals.PulleySlowFeedrate_mm_s(),
                mg::globals.PulleySlowFeedrate_mm_s());
        }
        return false;
    case PushingFilamentFast:
        if (mfs::fsensor.Pressed()) {
            // Safety precaution - if the fsensor triggers while pushing the filament fast, we must stop pushing immediately
            // With a correctly set-up MMU this shouldn't happen
            mm::motion.AbortPlannedMoves(); // stop pushing filament
            state = FSensorTooEarly;
        } else if (mm::motion.PlannedMoves(mm::Pulley) == 1) {
            // a bit of a hack - the fast (already planned) move has finished, doing the slow part
            // -> just switch to FeedingToFSensor
            state = PushingFilamentToFSensor;
        }
        return false;
    case PushingFilamentToFSensor:
        //dbg_logic_P(PSTR("Feed to Bondtech --> Pushing"));
        if (mfs::fsensor.Pressed()) {
            mm::motion.AbortPlannedMoves(); // stop pushing filament
            GoToPushToNozzle();
            //        } else if (mm::motion.StallGuard(mm::Pulley)) {
            //            // StallGuard occurred during movement - the filament got stuck
            //            state = PulleyStalled;
        } else if (mm::motion.QueueEmpty()) { // all moves have been finished and the fsensor didn't switch on
            state = Failed;
        }
        return false;
    case PushingFilamentIntoNozzle:
        if (mm::motion.QueueEmpty()) {
            mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::InNozzle);
            mi::idler.PartiallyDisengage(mg::globals.ActiveSlot());
            // while disengaging the idler, keep on moving with the pulley to avoid grinding while the printer is trying to grab the filament itself
            mpu::pulley.PlanMove(config::fsensorToNozzleAvoidGrind, config::pulleySlowFeedrate);
            state = PartiallyDisengagingIdler;
        }
        return false;
    case PartiallyDisengagingIdler:
        if (mi::idler.PartiallyDisengaged()) {
            mm::motion.AbortPlannedMoves(mm::Pulley);
            mpu::pulley.Disable();
            mi::idler.Disengage(); // disengage fully while Pulley is already stopped
            state = DisengagingIdler;
        }
        return false;
    case DisengagingIdler:
        if (mi::idler.Disengaged()) {
            dbg_logic_P(PSTR("Feed to Bondtech --> Idler disengaged"));
            dbg_logic_fP(PSTR("Pulley end steps %u"), mpu::pulley.CurrentPosition_mm());
            state = OK;
            ml::leds.SetMode(mg::globals.ActiveSlot(), ml::green, ml::on);
        }
        return false;
    case OK:
        dbg_logic_P(PSTR("Feed to Bondtech OK"));
        return true;
    case Failed:
    case FSensorTooEarly:
        //    case PulleyStalled:
        dbg_logic_P(PSTR("Feed to Bondtech FAILED"));
        return true;
    default:
        return true;
    }
}

} // namespace logic
