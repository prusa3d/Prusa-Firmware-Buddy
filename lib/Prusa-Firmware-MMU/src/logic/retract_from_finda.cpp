/// @file retract_from_finda.cpp
#include "retract_from_finda.h"
#include "../modules/finda.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/pulley.h"
#include "../debug.h"

namespace logic {

void RetractFromFinda::Reset() {
    dbg_logic_P(PSTR("\nRetract from FINDA\n\n"));
    state = EngagingIdler;
    ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::blink0, ml::off);
    mi::idler.Engage(mg::globals.ActiveSlot());
}

bool RetractFromFinda::Step() {
    switch (state) {
    case EngagingIdler:
        if (mi::idler.Engaged()) {
            dbg_logic_fP(PSTR("Pulley start steps %u"), mpu::pulley.CurrentPosition_mm());
            state = UnloadBackToPTFE;
            mpu::pulley.PlanMove(-(config::cuttingEdgeToFindaMidpoint + config::cuttingEdgeRetract), mg::globals.PulleyUnloadFeedrate_mm_s());
        }
        return false;
    case UnloadBackToPTFE:
        //dbg_logic_P(PSTR("Unload back to PTFE --> Pulling"));
        if (mm::motion.QueueEmpty()) { // all moves have been finished
            if (!mf::finda.Pressed()) { // FINDA switched off correctly while the move was performed
                state = OK;
                mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::AtPulley);
                dbg_logic_fP(PSTR("Pulley end steps %u"), mpu::pulley.CurrentPosition_mm());
                ml::leds.SetMode(mg::globals.ActiveSlot(), ml::green, ml::off);
            } else { // FINDA didn't switch off
                state = Failed;
                ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::off, ml::blink0);
            }
        }
        return false;
    case OK:
        dbg_logic_P(PSTR("Retract from FINDA OK"));
        return true;
    case Failed:
        dbg_logic_P(PSTR("Retract from FINDA FAILED"));
        return true;
    default:
        return true;
    }
}

} // namespace logic
