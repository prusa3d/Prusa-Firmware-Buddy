/**
 * @file selftest_firstlayer.hpp
 */
#pragma once

#include <inttypes.h>
#include "i_selftest_part.hpp"
#include "selftest_firstlayer_config.hpp"
#include "selftest_log.hpp"
#include "../../marlin_stubs/G26.hpp"

namespace selftest {

/**
 * @brief Object handling states of first layer calibration
 *
 * Simplified state-change map of first layer
 *
 * AskFilament --> Calib / Preheat / Load / Unload
 * Calib       --> Finish / Calib
 * Preheat     --> Calib
 * Load        --> Calib / AskFilament
 * Unload      --> AskFilament
 */
class CSelftestPart_FirstLayer {
    enum class StateSelectedByUser {
        Calib, // run calibration with current filament
        Preheat, // open preheat dialog to force set filament
        Load, // load new filament
        Unload // unload filament
    };

    IPartHandler &rStateMachine;
    const FirstLayerConfig_t &rConfig;
    SelftestFirstLayer_t &rResult;

    int temp_nozzle_preheat;
    int temp_bed;

    uint32_t how_many_times_finished;
    bool skip_user_changing_initial_distance;
    bool reprint;
    StateSelectedByUser state_selected_by_user;

    LogTimer log;
    FirstLayerProgressLock lock; // this ensures properly working progress
    bool enqueueGcode(const char *gcode) const;

public:
    CSelftestPart_FirstLayer(IPartHandler &state_machine, const FirstLayerConfig_t &config,
        SelftestFirstLayer_t &result);

    LoopResult stateStart();
    LoopResult stateCycleMark() { return LoopResult::MarkLoop0; }
    LoopResult stateAskFilamentInit();
    LoopResult stateAskFilament();
    LoopResult statePreheatEnqueueGcode();
    LoopResult statePreheatWaitFinished();
    LoopResult stateFilamentLoadEnqueueGcode();
    LoopResult stateFilamentLoadWaitFinished();
    LoopResult stateFilamentUnloadEnqueueGcode();
    LoopResult stateFilamentUnloadWaitFinished();
    LoopResult stateShowCalibrateMsg(); // this states require stateHandleNext
    LoopResult stateInitialDistanceInit();
    LoopResult stateInitialDistance();
    LoopResult stateShowStartPrint(); // this states require stateHandleNext

    LoopResult statePrintInit(); // this states require mark loop inserted before, to have functional reprint
    LoopResult stateWaitNozzle();
    LoopResult stateWaitBed();
    LoopResult stateHome();
    LoopResult stateMbl();
    LoopResult statePrint();

    LoopResult stateMblFinished();
    LoopResult statePrintFinished();
    LoopResult stateReprintInit();
    LoopResult stateReprint();
    LoopResult stateCleanSheetInit();
    LoopResult stateCleanSheet();

    LoopResult stateFinish();
    LoopResult stateHandleNext();
};

}; // namespace selftest
