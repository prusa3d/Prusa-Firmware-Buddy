#pragma once

#include <stdint.h>

#ifdef __cplusplus
//C++ checks enum classes

//Client finite state machines
enum class ClientFSM : uint8_t {
    Serial_printing,
    Load_unload,
    G162,
    SelftestAxis,
    SelftestFans,
    SelftestHeat,
    Printing, //not a dialog
    FirstLayer,
    _none, //cannot be created, must have same index as _count
    _count = _none
};

enum class LoadUnloadMode : uint8_t {
    Change,
    Load,
    Unload,
    Purge
};

enum class WarningType : uint32_t {
    HotendFanError,
    PrintFanError,
    HeaterTimeout,
    _last = HeaterTimeout
};

// Open dialog has a parameter because I need to set a caption of change filament dialog (load / unload / change).
// Use extra state of statemachine to set the caption would be cleaner, but I can miss events.
// Only the last sent event is guaranteed to pass its data.
using fsm_create_t = void (*)(ClientFSM, uint8_t);                                               //create finite state machine
using fsm_destroy_t = void (*)(ClientFSM);                                                       //destroy finite state machine
using fsm_change_t = void (*)(ClientFSM, uint8_t phase, uint8_t progress_tot, uint8_t progress); //change fsm state or progress
using message_cb_t = void (*)(const char *);
using warning_cb_t = void (*)(WarningType);
#else  // !__cplusplus
//C
typedef void (*fsm_create_t)(uint8_t, uint8_t);                                               //create finite state machine
typedef void (*fsm_destroy_t)(uint8_t);                                                       //destroy finite state machine
typedef void (*fsm_change_t)(uint8_t, uint8_t phase, uint8_t progress_tot, uint8_t progress); //change fsm state or progress
typedef void (*message_cb_t)(const char *);
typedef void (*warning_cb_t)(uint32_t);
#endif //__cplusplus
