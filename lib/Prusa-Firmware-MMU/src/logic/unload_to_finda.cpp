/// @file unload_to_finda.cpp
#include "unload_to_finda.h"
#include "../modules/finda.h"
#include "../modules/fsensor.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/permanent_storage.h"
#include "../modules/pulley.h"

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
            mpu::pulley.InitAxis();
            ml::leds.SetMode(mg::globals.ActiveSlot(), ml::green, ml::blink0);
        } else {
            state = FailedFINDA;
        }
        return false;
    case UnloadingToFinda:
        if (mi::idler.Engaged()) {
            state = WaitingForFINDA;
            mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::InSelector);
            unloadStart_mm = mpu::pulley.CurrentPosition_mm();
            // We can always plan the unload move for the maximum allowed bowden length,
            // it should be even more reliable than doing just the specified bowden length:
            // - if the filament is slipping for some reason, planning a longer move will not stop in the middle of the bowden tube
            // - a faster unload (shorter than the specified bowden length) will be interrupted by FINDA turning off
            // - if FINDA is misaligned or faulty, the only issue will be, that the filament will be thrown behind the pulley
            //   which could have happened with the previous implementation as well, because default bowden length was set to 42cm
            mpu::pulley.PlanMove(-config::maximumBowdenLength - config::feedToFinda - config::filamentMinLoadedToMMU, mg::globals.PulleyUnloadFeedrate_mm_s());
        }
        return false;
    case WaitingForFINDA: {
        int32_t currentPulley_mm = mpu::pulley.CurrentPosition_mm();
        if ((abs(unloadStart_mm - currentPulley_mm) > mm::truncatedUnit(mg::globals.FSensorUnloadCheck_mm())) && mfs::fsensor.Pressed()) {
            // fsensor didn't trigger within the first fsensorUnloadCheckDistance mm -> stop pulling, something failed, report an error
            // This scenario should not be tried again - repeating it may cause more damage to filament + potentially more collateral damage
            state = FailedFSensor;
            mm::motion.AbortPlannedMoves(); // stop rotating the pulley
            ml::leds.SetMode(mg::globals.ActiveSlot(), ml::green, ml::off);
        } else if (!mf::finda.Pressed()) {
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
                state = FailedFINDA;
            }
        }
    }
        return false;
    case OK:
    case FailedFINDA:
    case FailedFSensor:
    default:
        return true;
    }
}

} // namespace logic
