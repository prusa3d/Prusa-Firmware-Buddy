// errors_list.h
// Intended as a list of MMU errors for the Buddy FW, therefore the structure
// of this file is similar as in 12_MINI
#pragma once
#include "inttypes.h"
#include "i18n.h"
#include "button_operations.h"

namespace MMU2 {

static constexpr uint8_t ERR_MMU_CODE = 4;

typedef enum : uint16_t {
    ERR_UNDEF = 0,

    ERR_MECHANICAL = 100,
    ERR_MECHANICAL_FINDA_DIDNT_TRIGGER,
    ERR_MECHANICAL_FINDA_FILAM_STUCK,
    ERR_MECHANICAL_FSENSOR_DIDNT_TRIGGER,
    ERR_MECHANICAL_FSENSOR_FILAM_STUCK,

    ERR_MECHANICAL_PULLEY_CANNOT_MOVE = 105,
    ERR_MECHANICAL_SELECTOR_CANNOT_HOME = 115,
    ERR_MECHANICAL_SELECTOR_CANNOT_MOVE = 116,
    ERR_MECHANICAL_IDLER_CANNOT_HOME = 125,
    ERR_MECHANICAL_IDLER_CANNOT_MOVE = 126,

    ERR_MECHANICAL_FSENSOR_TOO_EARLY = 106,


    ERR_TEMPERATURE = 200,
    ERR_TEMPERATURE_TMC_PULLEY_WARNING_TMC_TOO_HOT = 201,
    ERR_TEMPERATURE_TMC_SELECTOR_WARNING_TMC_TOO_HOT = 211,
    ERR_TEMPERATURE_TMC_IDLER_WARNING_TMC_TOO_HOT = 221,

    ERR_TEMPERATURE_TMC_PULLEY_TMC_OVERHEAT_ERROR = 202,
    ERR_TEMPERATURE_TMC_SELECTOR_TMC_OVERHEAT_ERROR = 212,
    ERR_TEMPERATURE_TMC_IDLER_TMC_OVERHEAT_ERROR = 222,


    ERR_ELECTRICAL = 300,
    ERR_ELECTRICAL_TMC_PULLEY_DRIVER_ERROR = 301,
    ERR_ELECTRICAL_TMC_SELECTOR_DRIVER_ERROR = 311,
    ERR_ELECTRICAL_TMC_IDLER_DRIVER_ERROR = 321,

    ERR_ELECTRICAL_TMC_PULLEY_DRIVER_RESET = 302,
    ERR_ELECTRICAL_TMC_SELECTOR_DRIVER_RESET = 312,
    ERR_ELECTRICAL_TMC_IDLER_DRIVER_RESET = 322,

    ERR_ELECTRICAL_TMC_PULLEY_UNDERVOLTAGE_ERROR = 303,
    ERR_ELECTRICAL_TMC_SELECTOR_UNDERVOLTAGE_ERROR = 313,
    ERR_ELECTRICAL_TMC_IDLER_UNDERVOLTAGE_ERROR = 323,

    ERR_ELECTRICAL_TMC_PULLEY_DRIVER_SHORTED = 304,
    ERR_ELECTRICAL_TMC_SELECTOR_DRIVER_SHORTED = 314,
    ERR_ELECTRICAL_TMC_IDLER_DRIVER_SHORTED = 324,
    
    ERR_ELECTRICAL_PULLEY_SELFTEST_FAILED = 305,
    ERR_ELECTRICAL_SELECTOR_SELFTEST_FAILED = 315,
    ERR_ELECTRICAL_IDLER_SELFTEST_FAILED = 325,


    ERR_CONNECT = 400,
    ERR_CONNECT_MMU_NOT_RESPONDING = 401,
    ERR_CONNECT_COMMUNICATION_ERROR = 402,


    ERR_SYSTEM = 500,
    ERR_SYSTEM_FILAMENT_ALREADY_LOADED = 501,
    ERR_SYSTEM_INVALID_TOOL = 502,
    ERR_SYSTEM_QUEUE_FULL = 503,
    ERR_SYSTEM_FW_UPDATE_NEEDED = 504,
    ERR_SYSTEM_FW_RUNTIME_ERROR = 505,
    ERR_SYSTEM_UNLOAD_MANUALLY = 506,

    ERR_OTHER = 900
} err_num_t;

struct MMUErrorDesc {
    // 32 bit
    const char *err_title;
    const char *err_text;
    // 16 bit
    err_num_t err_num;
    std::array<ButtonOperations, 3> buttons;
};

static constexpr MMUErrorDesc error_list[] = {
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // MECHANICAL
    
    // r=1, c=20
    { N_("FINDA DIDNT TRIGGER"),
        // r=5, c=20
        N_("FINDA didn't trigger while loading the filament. Ensure the filament can move and FINDA works."),
        ERR_MECHANICAL_FINDA_DIDNT_TRIGGER,
      { ButtonOperations::NoOperation, ButtonOperations::Retry, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("FINDA: FILAM. STUCK"),
        // r=5, c=20
        N_("FINDA didn't switch off while unloading filament. Try unloading manually. Ensure filament can move and FINDA works."),
        ERR_MECHANICAL_FINDA_FILAM_STUCK,
      { ButtonOperations::NoOperation, ButtonOperations::Retry, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("FSENSOR DIDNT TRIGGER"),
        // r=5, c=20
        N_("Filament sensor didn't trigger while loading the filament. Ensure the filament reached the fsensor and the sensor works."),
        ERR_MECHANICAL_FSENSOR_DIDNT_TRIGGER,
      { ButtonOperations::NoOperation, ButtonOperations::Retry, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("FSENSOR: FIL. STUCK"),
        // r=5, c=20
        N_("Filament sensor didn't switch off while unloading filament. Ensure filament can move and the sensor works."),
        ERR_MECHANICAL_FSENSOR_FILAM_STUCK,
      { ButtonOperations::NoOperation, ButtonOperations::Retry, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("PULLEY CANNOT MOVE"),
        // r=5, c=20
        N_("Pulley motor stalled. Ensure the pulley can move and check the wiring."),
        ERR_MECHANICAL_PULLEY_CANNOT_MOVE,
      { ButtonOperations::NoOperation, ButtonOperations::Retry, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("FSENSOR TOO EARLY"),
        // r=5, c=20
        N_("Filament sensor triggered too early while loading to extruder. Check there isn't anything stuck in PTFE tube. Check that sensor reads properly."),
        ERR_MECHANICAL_FSENSOR_TOO_EARLY,
        { ButtonOperations::NoOperation, ButtonOperations::Retry, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("SELECTOR CANNOT HOME"),
        // r=5, c=20
        N_("The Selector cannot home properly. Check for anything blocking its movement."),
        ERR_MECHANICAL_SELECTOR_CANNOT_HOME,
      { ButtonOperations::NoOperation, ButtonOperations::Retry, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("SELECTOR CANNOT MOVE"),
        // r=5, c=20
        N_("The Selector cannot move. Check for anything blocking its movement. Check the wiring is correct."),
        ERR_MECHANICAL_SELECTOR_CANNOT_MOVE,
        { ButtonOperations::NoOperation, ButtonOperations::Retry, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("IDLER CANNOT HOME"),
        // r=5, c=20
        N_("The Idler cannot home properly. Check for anything blocking its movement."),
        ERR_MECHANICAL_IDLER_CANNOT_HOME,
      { ButtonOperations::NoOperation, ButtonOperations::Retry, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("IDLER CANNOT MOVE"),
        // r=5, c=20
        N_("The Idler cannot move properly. Check for anything blocking its movement. Check the wiring is correct."),
        ERR_MECHANICAL_IDLER_CANNOT_MOVE,
      { ButtonOperations::NoOperation, ButtonOperations::Retry, ButtonOperations::NoOperation }
    },

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // TEMPERATURE

    // r=1, c=20
    { N_("WARNING TMC TOO HOT"),
        // r=5, c=20
        N_("TMC driver for the Pulley motor is almost overheating. Make sure there is sufficient airflow near the MMU board."),
        ERR_TEMPERATURE_TMC_PULLEY_WARNING_TMC_TOO_HOT,
        { ButtonOperations::Continue, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("WARNING TMC TOO HOT"),
        // r=5, c=20
        N_("TMC driver for the Selector motor is almost overheating. Make sure there is sufficient airflow near the MMU board."),
        ERR_TEMPERATURE_TMC_SELECTOR_WARNING_TMC_TOO_HOT,
      { ButtonOperations::Continue, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("WARNING TMC TOO HOT"),
        // r=5, c=20
        N_("TMC driver for the Idler motor is almost overheating. Make sure there is sufficient airflow near the MMU board."),
        ERR_TEMPERATURE_TMC_IDLER_WARNING_TMC_TOO_HOT,
      { ButtonOperations::Continue, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },
    
    // r=1, c=20
    { N_("TMC OVERHEAT ERROR"),
        // r=5, c=20
        N_("TMC driver for the Pulley motor is overheated. Cool down the MMU board and reset MMU."),
        ERR_TEMPERATURE_TMC_PULLEY_TMC_OVERHEAT_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("TMC OVERHEAT ERROR"),
        // r=5, c=20
        N_("TMC driver for the Selector motor is overheated. Cool down the MMU board and reset MMU."),
        ERR_TEMPERATURE_TMC_SELECTOR_TMC_OVERHEAT_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("TMC OVERHEAT ERROR"),
        // r=5, c=20
        N_("TMC driver for the Idler motor is overheated. Cool down the MMU board and reset MMU."),
        ERR_TEMPERATURE_TMC_IDLER_TMC_OVERHEAT_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // ELECTRICAL
    
    // r=1, c=20
    { N_("TMC DRIVER ERROR"),
        // r=5, c=20
        N_("TMC driver for the Pulley motor is not responding. Try resetting the MMU. If the issue persists contact support."),
        ERR_ELECTRICAL_TMC_PULLEY_DRIVER_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("TMC DRIVER ERROR"),
        // r=5, c=20
        N_("TMC driver for the Selector motor is not responding. Try resetting the MMU. If the issue persists contact support."),
        ERR_ELECTRICAL_TMC_SELECTOR_DRIVER_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("TMC DRIVER ERROR"),
        // r=5, c=20
        N_("TMC driver for the Idler motor is not responding. Try resetting the MMU. If the issue persists contact support."),
        ERR_ELECTRICAL_TMC_IDLER_DRIVER_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("TMC DRIVER RESET"),
        // r=5, c=20
        N_("TMC driver for the Pulley motor was restarted. There is probably an issue with the electronics. Check the wiring and connectors."),
        ERR_ELECTRICAL_TMC_PULLEY_DRIVER_RESET,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },
    
    // r=1, c=20
    { N_("TMC DRIVER RESET"),
        // r=5, c=20
        N_("TMC driver for the Selector motor was restarted. There is probably an issue with the electronics. Check the wiring and connectors."),
        ERR_ELECTRICAL_TMC_SELECTOR_DRIVER_RESET,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("TMC DRIVER RESET"),
        // r=5, c=20
        N_("TMC driver for the Idler motor was restarted. There is probably an issue with the electronics. Check the wiring and connectors."),
        ERR_ELECTRICAL_TMC_IDLER_DRIVER_RESET,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },
    
    // r=1, c=20
    { N_("TMC UNDERVOLTAGE ERROR"),
        // r=5, c=20
        N_("Not enough current for the Pulley TMC driver. There is probably an issue with the electronics. Check the wiring and connectors."),
        ERR_ELECTRICAL_TMC_PULLEY_UNDERVOLTAGE_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("TMC UNDERVOLTAGE ERROR"),
        // r=5, c=20
        N_("Not enough current for the Selector TMC driver. There is probably an issue with the electronics. Check the wiring and connectors."),
        ERR_ELECTRICAL_TMC_SELECTOR_UNDERVOLTAGE_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("TMC UNDERVOLTAGE ERROR"),
        // r=5, c=20
        N_("Not enough current for the Idler TMC driver. There is probably an issue with the electronics. Check the wiring and connectors."),
        ERR_ELECTRICAL_TMC_IDLER_UNDERVOLTAGE_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },
    
    // r=1, c=20
    { N_("TMC DRIVER SHORTED"),
        // r=5, c=20
        N_("Short circuit on the Pulley TMC driver. Check the wiring and connectors. If the issue persists contact support."),
        ERR_ELECTRICAL_TMC_PULLEY_DRIVER_SHORTED,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("TMC DRIVER SHORTED"),
        // r=5, c=20
        N_("Short circuit on the Selector TMC driver. Check the wiring and connectors. If the issue persists contact support."),
        ERR_ELECTRICAL_TMC_SELECTOR_DRIVER_SHORTED,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("TMC DRIVER SHORTED"),
        // r=5, c=20
        N_("Short circuit on the Idler TMC driver. Check the wiring and connectors. If the issue persists contact support."),
        ERR_ELECTRICAL_TMC_IDLER_DRIVER_SHORTED,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("MMU SELFTEST FAILED"),
        // r=5, c=20
        N_("MMU selftest failed on the Pulley TMC driver. Check the wiring and connectors. If the issue persists contact support."),
        ERR_ELECTRICAL_PULLEY_SELFTEST_FAILED,
        { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },
    
    // r=1, c=20
    { N_("MMU SELFTEST FAILED"),
        // r=5, c=20
        N_("MMU selftest failed on the Selector TMC driver. Check the wiring and connectors. If the issue persists contact support."),
        ERR_ELECTRICAL_SELECTOR_SELFTEST_FAILED,
        { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },
    
    // r=1, c=20
    { N_("MMU SELFTEST FAILED"),
        // r=5, c=20
        N_("MMU selftest failed on the Idler TMC driver. Check the wiring and connectors. If the issue persists contact support."),
        ERR_ELECTRICAL_IDLER_SELFTEST_FAILED,
        { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // COMMUNICATION
    
    // r=1, c=20
    { N_("MMU NOT RESPONDING"),
        // r=5, c=20
        N_("MMU unit not responding. Check the wiring and connectors. If the issue persists, contact support."),
        ERR_CONNECT_MMU_NOT_RESPONDING,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },
    
    // r=1, c=20
    { N_("COMMUNICATION ERROR"),
        // r=5, c=20
        N_("MMU unit not responding correctly. Check the wiring and connectors. If the issue persists, contact support."),
        ERR_CONNECT_COMMUNICATION_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM
    
    // r=1, c=20
    { N_("FILAMENT ALREADY LOADED"),
        // r=5, c=20
        N_("Cannot perform the action, filament is already loaded. Unload it first."),
        ERR_SYSTEM_FILAMENT_ALREADY_LOADED, 
        { ButtonOperations::Unload, ButtonOperations::Continue, ButtonOperations::NoOperation }
    },
    
    // r=1, c=20
    { N_("INVALID TOOL"),
        // r=5, c=20
        N_("Requested filament tool is not available on this hardware. Check the G-code for tool index out of range (T0-T4)."),
        ERR_SYSTEM_INVALID_TOOL, 
        { ButtonOperations::StopPrint, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("QUEUE FULL"),
        // r=5, c=20
        N_("MMU Firmware internal error, please reset the MMU."),
        ERR_SYSTEM_QUEUE_FULL, 
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("FW UPDATE NEEDED"),
        // r=5, c=20
        N_("The MMU unit reports its FW version incompatible with the printer's firmware. Make sure the MMU firmware is up to date."),
        ERR_SYSTEM_FW_UPDATE_NEEDED, 
      { ButtonOperations::NoOperation, ButtonOperations::DisableMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=20
    { N_("FW RUNTIME ERROR"),
        // r=5, c=20
        N_("Internal runtime error. Try resetting the MMU unit or updating the firmware. If the issue persists, contact support."),
        ERR_SYSTEM_FW_RUNTIME_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::ResetMMU, ButtonOperations::NoOperation }
    },
    
    // r=1, c=20
    { N_("UNLOAD MANUALLY"),
        // r=5, c=20
        N_("Unexpected FINDA reading. Ensure no filament is under FINDA and the selector is free. Check FINDA connection."),
        ERR_SYSTEM_FW_RUNTIME_ERROR,
        { ButtonOperations::NoOperation, ButtonOperations::Retry, ButtonOperations::NoOperation }
    }
};

} // namespace MMU2
