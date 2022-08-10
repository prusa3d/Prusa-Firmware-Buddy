/**
 * @file selftest_firstlayer.hpp
 */
#pragma once

#include <inttypes.h>
#include "i_selftest_part.hpp"
#include "selftest_firstlayer_config.hpp"
#include "selftest_log.hpp"

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
    enum {
        FKNOWN = 0x01,      //filament is known
        F_NOTSENSED = 0x02, //filament is not in sensor
    };

    enum class StateSelectedByUser {
        Calib,
        Preheat,
        Load,
        Unload
    };

    IPartHandler &rStateMachine;
    const FirstLayerConfig_t &rConfig;
    SelftestFirstLayer_t &rResult;

    uint32_t how_many_times_finished;
    bool filament_known_but_unsensed;
    bool current_offset_is_default;
    bool reprint;
    StateSelectedByUser state_selected_by_user;

    LogTimer log;

public:
    CSelftestPart_FirstLayer(IPartHandler &state_machine, const FirstLayerConfig_t &config,
        SelftestFirstLayer_t &result);

    LoopResult stateStart();
    LoopResult stateCycleMark() { return LoopResult::MarkLoop; }
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
    LoopResult statePrint();          // this states require mark loop before print, to have functional reprint
    LoopResult stateMblFinished();
    LoopResult statePrintFinished();
    LoopResult stateReprintInit();
    LoopResult stateReprint();
    LoopResult stateCleanSheetInit();
    LoopResult stateCleanSheet();

    LoopResult stateFinish();
    LoopResult stateHandleNext();
};

};
