/// @file feed_to_finda.cpp
#include "feed_to_finda.h"
#include "../modules/finda.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/selector.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/permanent_storage.h"
#include "../modules/user_input.h"
#include "../debug.h"
namespace logic {

void FeedToFinda::Reset(bool feedPhaseLimited) {
    dbg_logic_P(PSTR("\nFeed to FINDA\n\n"));
    state = EngagingIdler;
    this->feedPhaseLimited = feedPhaseLimited;
    ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::blink0, ml::off);
    mi::idler.Engage(mg::globals.ActiveSlot());
    // We can't get any FINDA readings if the selector is at the wrong spot - move it accordingly if necessary
    ms::selector.MoveToSlot(mg::globals.ActiveSlot());
}

bool FeedToFinda::Step() {
    switch (state) {
    case EngagingIdler:
        if (mi::idler.Engaged() && ms::selector.Slot() == mg::globals.ActiveSlot()) {
            dbg_logic_P(PSTR("Feed to Finda --> Idler engaged"));
            dbg_logic_fP(PSTR("Pulley start steps %u"), mm::motion.CurPosition(mm::Pulley));
            state = PushingFilament;
            mm::motion.InitAxis(mm::Pulley);
            // @@TODO this may never happen as load filament always assumes the filament is at least at the pulley
            //            if (mg::globals.FilamentLoaded() == mg::FilamentLoadState::NotLoaded) { // feed slowly filament to PTFE
            //                mm::motion.PlanMove<mm::Pulley>(config::filamentMinLoadedToMMU, config::pulleySlowFeedrate);
            //            }
            mm::motion.PlanMove<mm::Pulley>(config::feedToFinda, config::pulleyFeedrate);
            mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::InSelector);
            mui::userInput.Clear(); // remove all buffered events if any just before we wait for some input
        }
        return false;
    case PushingFilament: {
        if (mf::finda.Pressed() || (feedPhaseLimited && mui::userInput.AnyEvent())) { // @@TODO probably also a command from the printer
            mm::motion.AbortPlannedMoves(); // stop pushing filament
            // FINDA triggered - that means it works and detected the filament tip
            mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::InSelector);
            dbg_logic_P(PSTR("Feed to Finda --> Idler disengaged"));
            dbg_logic_fP(PSTR("Pulley end steps %u"), mm::motion.CurPosition(mm::Pulley));
            state = OK;
        } else if (mm::motion.QueueEmpty()) { // all moves have been finished and FINDA didn't switch on
            state = Failed;
            ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::off, ml::blink0);
        }
    }
        return false;
    case OK:
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
