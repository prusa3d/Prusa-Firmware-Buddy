#pragma once

#include "marlin_events.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

typedef enum {
    DLG_serial_printing
} dialog_t;

typedef void (*dialog_open_cb_t)(dialog_t);  //open dialog
typedef void (*dialog_close_cb_t)(dialog_t); //close dialog

#ifdef __cplusplus
}
#endif //__cplusplus
