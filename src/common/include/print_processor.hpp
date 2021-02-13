/**
 * @file print_processor.hpp
 * @author Radek Vana
 * @brief api header marlin, meant to be switched with other in unit tests
 * @date 2021-02-11
 */

#pragma once

#include <stdint.h>

namespace {
#include "marlin_client.h"
};
#include "filament_sensor.hpp"

class PrintProcessor {
    //called when Serial print screen is opened
    //printer is not in sd printing mode, so filament sensor does not trigger M600
    //todo should I block ClientFSM::Serial_printing?
    //this code did not work in last builds and no one reported problem with octoscreen
    //I fear enabling it could break something
    static void fsm_create_cb(ClientFSM fsm, uint8_t data) {
        if (/*fsm == ClientFSM::Serial_printing ||*/ fsm == ClientFSM::Load_unload)
            FS_instance().IncEvLock();
    }
    static void fsm_destroy_cb(ClientFSM fsm) {
        if (/*fsm == ClientFSM::Serial_printing ||*/ fsm == ClientFSM::Load_unload)
            FS_instance().DecEvLock();
    }

public:
    static inline void Update() { marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_SD_PRINT) | MARLIN_VAR_MSK(MARLIN_VAR_FS_AUTOLOAD_ENABLED)); }
    static inline void InjectGcode(const char *str) { marlin_gcode_push_front(str); }
    static inline bool IsPrinting() { return marlin_vars()->sd_printing; }
    static inline bool IsAutoloadEnabled() { return marlin_vars()->fs_autoload_enabled; }

    static void Init() {
        marlin_client_set_fsm_create_cb(fsm_create_cb);
        marlin_client_set_fsm_destroy_cb(fsm_destroy_cb);
    }
};
