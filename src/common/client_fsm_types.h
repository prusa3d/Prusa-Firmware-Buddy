#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//Client finite state machines
typedef enum {
    FSM_serial_printing,
    FSM_load_unload,
    FSM_no_dialog, //cannot be created, must have same index as FSM_count
    FSM_count = FSM_no_dialog
} ClinetFSM;

typedef enum {
    DLG_type_change,
    DLG_type_load,
    DLG_type_unload
} load_unload_type_t;

//open dialog has paramener
//because I need to set caption of change filament dialog (load / unload / change)
//use extra state of statemachine to set caption woud be cleaner, but I can miss events
//only last sent event is guaranteed  to pass its data
typedef void (*dialog_open_cb_t)(ClinetFSM, uint8_t);                                                 //open dialog
typedef void (*dialog_close_cb_t)(ClinetFSM);                                                         //close dialog
typedef void (*dialog_change_cb_t)(ClinetFSM, uint8_t phase, uint8_t progress_tot, uint8_t progress); //change dialog state or progress

#ifdef __cplusplus
}
#endif //__cplusplus
