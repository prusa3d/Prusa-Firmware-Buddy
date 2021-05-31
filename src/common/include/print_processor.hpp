/**
 * @file print_processor.hpp
 * @author Radek Vana
 * @brief api header marlin, meant to be switched with other in unit tests
 * @date 2021-02-11
 */

#pragma once

#include <stdint.h>
#include "marlin_client.h"
#include "filament_sensor.hpp"
#include "fsm_types.hpp"

class PrintProcessor {
    //called when Serial print screen is opened
    //printer is not in sd printing mode, so filament sensor does not trigger M600
    //todo should I block ClientFSM::Serial_printing?
    //this code did not work in last builds and no one reported problem with octoscreen
    //I fear enabling it could break something
    static void fsm_cb(uint32_t u32, uint16_t u16) {
        fsm::variant_t variant(u32, u16);
        if (/*variant.GetType() == ClientFSM::Serial_printing ||*/ variant.GetType() == ClientFSM::Load_unload) {
            if (variant.GetCommand() == ClientFSM_Command::create) {
                FS_instance().IncEvLock();
            }
            if (variant.GetCommand() == ClientFSM_Command::destroy) {
                FS_instance().DecEvLock();
            }
        }
    }

public:
    static inline void Update() { marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_SD_PRINT) | MARLIN_VAR_MSK(MARLIN_VAR_FS_AUTOLOAD_ENABLED)); }
    static inline void InjectGcode(const char *str) { marlin_gcode_push_front(str); }
    static inline bool IsPrinting() { return marlin_vars()->sd_printing; }
    static inline bool IsAutoloadEnabled() { return marlin_vars()->fs_autoload_enabled; }

    static void Init() {
        marlin_client_set_fsm_cb(fsm_cb);
    }
};
