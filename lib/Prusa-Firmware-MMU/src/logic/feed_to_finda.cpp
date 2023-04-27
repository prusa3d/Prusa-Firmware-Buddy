/// @file feed_to_finda.cpp
#include "feed_to_finda.h"
#include "../modules/finda.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/selector.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/permanent_storage.h"
#include "../modules/pulley.h"
#include "../modules/user_input.h"
#include "../debug.h"
namespace logic {

bool FeedToFinda::Reset(bool feedPhaseLimited, bool haltAtEnd) {
    dbg_logic_P(PSTR("\nFeed to FINDA\n\n"));
    state = EngagingIdler;
    this->feedPhaseLimited = feedPhaseLimited;
    this->haltAtEnd = haltAtEnd;
    ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::blink0, ml::off);
    if (ms::selector.MoveToSlot(mg::globals.ActiveSlot()) != ms::Selector::OperationResult::Accepted) {
        // We can't get any FINDA readings if the selector is at the wrong spot - move it accordingly if necessary
        // And prevent issuing any commands to the idler in such an error state
        return false;
    } else {
        // Selector accepted the command, we can plan the Idler as well
        mi::idler.Engage(mg::globals.ActiveSlot());
        return true;
    }
}

bool FeedToFinda::Step() {
    switch (state) {
    case EngagingIdler:
        // A serious deadlock may occur at this spot in case of flickering FINDA.
        // Therefore FeedToFinda::Reset returns false in case of Selector refusing to move.
        // We don't have to check the FINDA state while the move is in progress.
        if (mi::idler.Engaged() && ms::selector.Slot() == mg::globals.ActiveSlot()) {
            dbg_logic_P(PSTR("Feed to Finda --> Idler engaged"));
            dbg_logic_fP(PSTR("Pulley start steps %u"), mpu::pulley.CurrentPosition_mm());
            mpu::pulley.InitAxis();
            // @@TODO this may never happen as load filament always assumes the filament is at least at the pulley
            //            if (mg::globals.FilamentLoaded() == mg::FilamentLoadState::NotLoaded) { // feed slowly filament to PTFE
            //                mpu::pulley.PlanMove(config::filamentMinLoadedToMMU, config::pulleySlowFeedrate);
            //            }

            mpu::pulley.PlanMove(config::maximumFeedToFinda, config::pulleySlowFeedrate);
            if (feedPhaseLimited) {
                state = PushingFilament;
            } else {
                state = PushingFilamentUnlimited;
                // in unlimited move we plan 2 moves at once to make the move "seamless"
                // one move has already been planned above, this is the second one
                mpu::pulley.PlanMove(config::maximumFeedToFinda, config::pulleySlowFeedrate);
            }
            mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::InSelector);
            mui::userInput.Clear(); // remove all buffered events if any just before we wait for some input
        }
        return false;
    case PushingFilament: {
        if (mf::finda.Pressed()) {
            mm::motion.AbortPlannedMoves(haltAtEnd); // stop pushing filament
            // FINDA triggered - that means it works and detected the filament tip
            mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::InSelector);
            dbg_logic_P(PSTR("Feed to Finda --> Idler disengaged"));
            dbg_logic_fP(PSTR("Pulley end steps %u"), mpu::pulley.CurrentPosition_mm());
            state = OK;
            return true; // return immediately to allow for a seamless planning of another move (like feeding to bondtech)
        } else if (mm::motion.QueueEmpty()) { // all moves have been finished and FINDA didn't switch on
            state = Failed;
            ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::off, ml::blink0);
        }
    }
        return false;
    case PushingFilamentUnlimited: {
        // FINDA triggered or the user pressed a button -> stop move, consider the operation as finished ok
        if (mf::finda.Pressed() || mui::userInput.AnyEvent()) {
            mm::motion.AbortPlannedMoves(haltAtEnd); // stop pushing filament
            // FINDA triggered - that means it works and detected the filament tip
            mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::InSelector);
            dbg_logic_P(PSTR("Feed to Finda --> Idler disengaged"));
            dbg_logic_fP(PSTR("Pulley end steps %u"), mpu::pulley.CurrentPosition_mm());
            state = mui::userInput.AnyEvent() ? Stopped : OK;
            mui::userInput.Clear();
            return true; // return immediately to allow for a seamless planning of another move (like feeding to bondtech)
        } else if (mm::motion.PlannedMoves(mm::Pulley) < 2) {
            // plan another move to make the illusion of unlimited moves
            mpu::pulley.PlanMove(config::maximumFeedToFinda, config::pulleySlowFeedrate);
        }
    }
        return false;
    case OK:
    case Stopped:
        dbg_logic_P(PSTR("Feed to FINDA OK"));
        return true;
    case Failed:
        dbg_logic_P(PSTR("Feed to FINDA FAILED"));
        return true;
    default:
        return true;
    }
}

} // namespace logic
