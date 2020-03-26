#pragma once

#include <stdint.h>

#ifdef __cplusplus
//C++ checks enum clases

//Client finite state machines
enum class ClinetFSM : uint8_t {
    Serial_printing,
    Load_unload,
    _no_dialog, //cannot be created, must have same index as _count
    _count = _no_dialog
};

enum class LoadUnloadMode : uint8_t {
    Change,
    Load,
    Unload
};

//open dialog has paramener
//because I need to set caption of change filament dialog (load / unload / change)
//use extra state of statemachine to set caption woud be cleaner, but I can miss events
//only last sent event is guaranteed  to pass its data
using dialog_open_cb_t = void (*)(ClinetFSM, uint8_t);                                                 //open dialog
using dialog_close_cb_t = void (*)(ClinetFSM);                                                         //close dialog
using dialog_change_cb_t = void (*)(ClinetFSM, uint8_t phase, uint8_t progress_tot, uint8_t progress); //change dialog state or progress

#else  // !__cplusplus
//C
typedef void (*dialog_open_cb_t)(uint8_t, uint8_t);                                                 //open dialog
typedef void (*dialog_close_cb_t)(uint8_t);                                                         //close dialog
typedef void (*dialog_change_cb_t)(uint8_t, uint8_t phase, uint8_t progress_tot, uint8_t progress); //change dialog state or progress
#endif //__cplusplus
