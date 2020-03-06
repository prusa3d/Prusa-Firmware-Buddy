#pragma once

#include "marlin_events.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

typedef enum {
    DLG_serial_printing
} dialog_t;

//open dialog has paramener
//because I need to set caption of change filament dialog (load / unload / change)
//use extra state of statemachine to set caption woud be cleaner, but I can miss events
//only last sent event is guaranteed  to pass its data
typedef void (*dialog_open_cb_t)(dialog_t, uint8_t); //open dialog
typedef void (*dialog_close_cb_t)(dialog_t);         //close dialog

#ifdef __cplusplus
}
#endif //__cplusplus
