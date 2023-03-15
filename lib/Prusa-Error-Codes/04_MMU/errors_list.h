// errors_list.h
// Intended as a list of MMU errors for the Buddy FW, therefore the structure
// of this file is similar as in 12_MINI
#pragma once
#include "inttypes.h"
#include "i18n.h"

namespace MMU2 {

static constexpr uint8_t ERR_MMU_CODE = 4;

typedef enum : uint16_t {
    ERR_UNDEF = 0,

    ERR_MECHANICAL = 100,
    ERR_MECHANICAL_FINDA_DIDNT_TRIGGER,
    ERR_MECHANICAL_FINDA_DIDNT_SWITCH_OFF,
    ERR_MECHANICAL_FSENSOR_DIDNT_TRIGGER,
    ERR_MECHANICAL_FSENSOR_DIDNT_SWITCH_OFF,

    ERR_MECHANICAL_PULLEY_STALLED = 105,
    ERR_MECHANICAL_SELECTOR_CANNOT_HOME = 115,
    ERR_MECHANICAL_IDLER_CANNOT_HOME = 125,


    ERR_TEMPERATURE = 200,
    ERR_TEMPERATURE_TMC_PULLEY_OVER_TEMPERATURE_WARN = 201,
    ERR_TEMPERATURE_TMC_SELECTOR_OVER_TEMPERATURE_WARN = 211,
    ERR_TEMPERATURE_TMC_IDLER_OVER_TEMPERATURE_WARN = 221,

    ERR_TEMPERATURE_TMC_PULLEY_OVER_TEMPERATURE_ERROR = 202,
    ERR_TEMPERATURE_TMC_SELECTOR_OVER_TEMPERATURE_ERROR = 212,
    ERR_TEMPERATURE_TMC_IDLER_OVER_TEMPERATURE_ERROR = 222,


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


    ERR_CONNECT = 400,
    ERR_CONNECT_MMU_NOT_RESPONDING = 401,
    ERR_CONNECT_COMMUNICATION_ERROR = 402,


    ERR_SYSTEM = 500,
    ERR_SYSTEM_FILAMENT_ALREADY_LOADED = 501,
    ERR_SYSTEM_INVALID_TOOL = 502,
    ERR_SYSTEM_QUEUE_FULL = 503,
    ERR_SYSTEM_VERSION_MISMATCH = 504,
    ERR_SYSTEM_RUNTIME_ERROR = 505,

    ERR_OTHER = 900
} err_num_t;

/// Will be mapped onto dialog button responses in the Buddy FW
/// Those responses have their unique+translated texts as well
enum class ButtonOperations : uint8_t {
    NoOperation,
    Retry,
    SlowLoad,
    Continue,
    RestartMMU,
    Unload,
    StopPrint,
    DisableMMU,
};

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
    
    // r=1, c=19
    { N_("FINDA DIDNT TRIGGER"),
        // r=5, c=20
        N_("FINDA didn't trigger while loading filament. Ensure that the steel FINDA ball can move freely and check the wiring."),
        ERR_MECHANICAL_FINDA_DIDNT_TRIGGER,
        { ButtonOperations::SlowLoad, ButtonOperations::Retry, ButtonOperations::Continue }
    },

    // r=1, c=19
    { N_("FINDA DIDNT SWITCH OFF"),
        // r=5, c=20
        N_("FINDA didn't switch off while unloading filament. Try unloading the filament manually and ensure steel FINDA ball can move freely."),
        ERR_MECHANICAL_FINDA_DIDNT_SWITCH_OFF,
      { ButtonOperations::SlowLoad, ButtonOperations::Retry, ButtonOperations::Continue }
    },

    // r=1, c=19
    { N_("FSENSOR DIDNT TRIGGER"),
        // r=5, c=20
        N_("Filament sensor didn't trigger while loading the filament. Check that the filament reached the fsensor and check the wiring."),
        ERR_MECHANICAL_FSENSOR_DIDNT_TRIGGER,
      { ButtonOperations::SlowLoad, ButtonOperations::Retry, ButtonOperations::Continue }
    },

    // r=1, c=19
    { N_("FSENSOR DIDNT SWITCH OFF"),
        // r=5, c=20
        N_("Filament sensor didn't switch off while unloading the filament. The filament is probably stuck near the sensor or the sensor is malfunctioning."),
        ERR_MECHANICAL_FSENSOR_DIDNT_SWITCH_OFF,
      { ButtonOperations::SlowLoad, ButtonOperations::Retry, ButtonOperations::Continue }
    },

    // r=1, c=19
    { N_("PULLEY STALLED"),
        // r=5, c=20
        N_("The Pulley stalled - check for anything blocking the filament from being pushed/pulled to/from the extruder."),
        ERR_MECHANICAL_PULLEY_STALLED,
      { ButtonOperations::NoOperation, ButtonOperations::Retry, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("SELECTOR CANNOT HOME"),
        // r=5, c=20
        N_("The Selector cannot home properly - check for anything blocking its movement."),
        ERR_MECHANICAL_SELECTOR_CANNOT_HOME,
      { ButtonOperations::NoOperation, ButtonOperations::Retry, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("IDLER CANNOT HOME"),
        // r=5, c=20
        N_("The Idler cannot home properly - check for anything blocking its movement."),
        ERR_MECHANICAL_IDLER_CANNOT_HOME,
      { ButtonOperations::NoOperation, ButtonOperations::Retry, ButtonOperations::NoOperation }
    },

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // TEMPERATURE
    
    // r=1, c=19
    { N_("TMC TOO HOT"),
        // r=5, c=20
        N_("TMC driver for the Pulley motor is almost overheated. Make sure there is sufficient airflow near the MMU board."),
        ERR_TEMPERATURE_TMC_PULLEY_OVER_TEMPERATURE_WARN,
        { ButtonOperations::Continue, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("TMC TOO HOT"),
        // r=5, c=20
        N_("TMC driver for the Selector motor is almost overheated. Make sure there is sufficient airflow near the MMU board."),
        ERR_TEMPERATURE_TMC_SELECTOR_OVER_TEMPERATURE_WARN,
      { ButtonOperations::Continue, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("TMC TOO HOT"),
        // r=5, c=20
        N_("TMC driver for the Idler motor is almost overheated. Make sure there is sufficient airflow near the MMU board."),
        ERR_TEMPERATURE_TMC_IDLER_OVER_TEMPERATURE_WARN,
      { ButtonOperations::Continue, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },
    
    // r=1, c=19
    { N_("TMC OVERHEAT ERROR"),
        // r=5, c=20
        N_("TMC driver for the Pulley motor is overheated. Cool down the MMU board and restart MMU."),
        ERR_TEMPERATURE_TMC_PULLEY_OVER_TEMPERATURE_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("TMC OVERHEAT ERROR"),
        // r=5, c=20
        N_("TMC driver for the Selector motor is overheated. Cool down the MMU board and restart MMU."),
        ERR_TEMPERATURE_TMC_SELECTOR_OVER_TEMPERATURE_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("TMC OVERHEAT ERROR"),
        // r=5, c=20
        N_("TMC driver for the Idler motor is overheated. Cool down the MMU board and restart MMU."),
        ERR_TEMPERATURE_TMC_IDLER_OVER_TEMPERATURE_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // ELECTRICAL
    
    // r=1, c=19
    { N_("TMC DRIVER ERROR"),
        // r=5, c=20
        N_("TMC driver for the Pulley motor is not responding. Try restarting the MMU. If the issue persist contact the support."),
        ERR_ELECTRICAL_TMC_PULLEY_DRIVER_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("TMC DRIVER ERROR"),
        // r=5, c=20
        N_("TMC driver for the Selector motor is not responding. Try restarting the MMU. If the issue persist contact the support."),
        ERR_ELECTRICAL_TMC_SELECTOR_DRIVER_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("TMC DRIVER ERROR"),
        // r=5, c=20
        N_("TMC driver for the Idler motor is not responding. Try restarting the MMU. If the issue persist contact the support."),
        ERR_ELECTRICAL_TMC_IDLER_DRIVER_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("TMC DRIVER RESET"),
        // r=5, c=20
        N_("TMC driver for the Pulley motor was restarted. There is probably an issue with the electronics. Check the wiring and connectors."),
        ERR_ELECTRICAL_TMC_PULLEY_DRIVER_RESET,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },
    
    // r=1, c=19
    { N_("TMC DRIVER RESET"),
        // r=5, c=20
        N_("TMC driver for the Selector motor was restarted. There is probably an issue with the electronics. Check the wiring and connectors."),
        ERR_ELECTRICAL_TMC_SELECTOR_DRIVER_RESET,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("TMC DRIVER RESET"),
        // r=5, c=20
        N_("TMC driver for the Idler motor was restarted. There is probably an issue with the electronics. Check the wiring and connectors."),
        ERR_ELECTRICAL_TMC_IDLER_DRIVER_RESET,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },
    
    // r=1, c=19
    { N_("TMC UNDERVOLTAGE ERROR"),
        // r=5, c=20
        N_("Not enough current for the Pulley TMC driver. There is probably an issue with the electronics. Check the wiring and connectors."),
        ERR_ELECTRICAL_TMC_PULLEY_UNDERVOLTAGE_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("TMC UNDERVOLTAGE ERROR"),
        // r=5, c=20
        N_("Not enough current for the Selector TMC driver. There is probably an issue with the electronics. Check the wiring and connectors."),
        ERR_ELECTRICAL_TMC_SELECTOR_UNDERVOLTAGE_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("TMC UNDERVOLTAGE ERROR"),
        // r=5, c=20
        N_("Not enough current for the Idler TMC driver. There is probably an issue with the electronics. Check the wiring and connectors."),
        ERR_ELECTRICAL_TMC_IDLER_UNDERVOLTAGE_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },
    
    // r=1, c=19
    { N_("TMC DRIVER SHORTED"),
        // r=5, c=20
        N_("Short circuit on the Pulley TMC driver. Check the wiring and connectors. If the issue persist contact the support."),
        ERR_ELECTRICAL_TMC_PULLEY_DRIVER_SHORTED,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("TMC DRIVER SHORTED"),
        // r=5, c=20
        N_("Short circuit on the Selector TMC driver. Check the wiring and connectors. If the issue persist contact the support."),
        ERR_ELECTRICAL_TMC_SELECTOR_DRIVER_SHORTED,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("TMC DRIVER SHORTED"),
        // r=5, c=20
        N_("Short circuit on the Idler TMC driver. Check the wiring and connectors. If the issue persist contact the support."),
        ERR_ELECTRICAL_TMC_IDLER_DRIVER_SHORTED,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // COMMUNICATION
    
    // r=1, c=19
    { N_("MMU NOT RESPONDING"),
        // r=5, c=20
        N_("MMU unit not responding. Check the wiring and connectors. If the issue persist contact the support."),
        ERR_CONNECT_MMU_NOT_RESPONDING,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },
    
    // r=1, c=19
    { N_("COMMUNICATION ERROR"),
        // r=5, c=20
        N_("MMU unit not responding correctly. Check the wiring and connectors. If the issue persist contact the support."),
        ERR_CONNECT_COMMUNICATION_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM
    
    // r=1, c=19
    { N_("FILAMENT ALREADY LOADED"),
        // r=5, c=20
        N_("Cannot perform the action, filament is already loaded. Unload it first."),
        ERR_SYSTEM_FILAMENT_ALREADY_LOADED, 
        { ButtonOperations::Unload, ButtonOperations::Continue, ButtonOperations::RestartMMU }
    },
    
    // r=1, c=19
    { N_("INVALID TOOL"),
        // r=5, c=20
        N_("Requested filament tool is not available on this hardware. Check the G-code file for possible issue."),
        ERR_SYSTEM_INVALID_TOOL, 
        { ButtonOperations::StopPrint, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("QUEUE FULL"),
        // r=5, c=20
        N_("Internal runtime error of the firmware, please restart the MMU."),
        ERR_SYSTEM_QUEUE_FULL, 
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("FW VERSION MISMATCH"),
        // r=5, c=20
        N_("The MMU unit reports its FW version incompatible with the printer's firmware. Make sure the MMU firmware is up to date."),
        ERR_SYSTEM_VERSION_MISMATCH, 
      { ButtonOperations::NoOperation, ButtonOperations::DisableMMU, ButtonOperations::NoOperation }
    },

    // r=1, c=19
    { N_("FW RUNTIME ERROR"),
        // r=5, c=20
        N_("Internal runtime error. Try restarting the MMU unit or updating the firmware. If the issue persist contact the support."),
        ERR_SYSTEM_RUNTIME_ERROR,
      { ButtonOperations::NoOperation, ButtonOperations::RestartMMU, ButtonOperations::NoOperation }
    }
};

} // namespace MMU2
