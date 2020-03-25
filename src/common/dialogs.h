#pragma once

#include "marlin_events.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

typedef enum {
    DLG_serial_printing,
    DLG_load_unload,
    DLG_no_dialog, //cannot be created, must have same index as DLG_count
    DLG_count = DLG_no_dialog
} dialog_t;

typedef enum {
    DLG_type_change,
    DLG_type_load,
    DLG_type_unload
} load_unload_type_t;

//open dialog has paramener
//because I need to set caption of change filament dialog (load / unload / change)
//use extra state of statemachine to set caption woud be cleaner, but I can miss events
//only last sent event is guaranteed  to pass its data
typedef void (*dialog_open_cb_t)(dialog_t, uint8_t);                                                 //open dialog
typedef void (*dialog_close_cb_t)(dialog_t);                                                         //close dialog
typedef void (*dialog_change_cb_t)(dialog_t, uint8_t phase, uint8_t progress_tot, uint8_t progress); //change dialog state or progress

#ifdef __cplusplus
}
#endif //__cplusplus
