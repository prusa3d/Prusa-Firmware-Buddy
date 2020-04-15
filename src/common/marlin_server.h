// marlin_server.h
#ifndef _MARLIN_SERVER_H
#define _MARLIN_SERVER_H

#include "marlin_events.h"
#include "marlin_vars.h"
#include "marlin_errors.h"
#include "marlin_host.h"
#include "client_fsm_types.h"

// server flags
#define MARLIN_SFLG_STARTED 0x0001 // server started (set in marlin_server_init)
#define MARLIN_SFLG_PROCESS 0x0002 // loop processing in main thread is enabled
#define MARLIN_SFLG_BUSY    0x0004 // loop is busy
#define MARLIN_SFLG_PENDREQ 0x0008 // pending request

// server variable update interval [ms]
#define MARLIN_UPDATE_PERIOD 100
#define MSG_STACK_SIZE       8  //status message stack size
#define MSG_MAX_LENGTH       21 //status message max length

typedef void(marlin_server_idle_t)(void);

#pragma pack(push)
#pragma pack(1)

typedef struct msg_stack {

    char msg_data[MSG_STACK_SIZE][MSG_MAX_LENGTH];
    uint8_t count;

} msg_stack_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern msg_stack_t msg_stack;

// callback for idle operation inside marlin (called from ExtUI handler onIdle)
extern marlin_server_idle_t *marlin_server_idle_cb;

//-----------------------------------------------------------------------------
// server side functions (can be called from server thread only)

// initialize server side - must be called at beginning in server thread
extern void marlin_server_init(void);

// server loop - must be called periodically in server thread
extern int marlin_server_loop(void);

// returns enabled status of loop processing
extern int marlin_server_processing(void);

// direct start loop processing
extern void marlin_server_start_processing(void);

// direct stop loop processing + disable heaters and safe state
extern void marlin_server_stop_processing(void);

// update variables at server side, defined by 'update' mask and send notification to all clients
extern void marlin_server_update(uint64_t update);

// set printing gcode name (for WUI)
extern void marlin_server_set_gcode_name(const char *request);

// get printing gcode name (for WUI)
extern void marlin_server_get_gcode_name(const char *dest);

// direct call of babystep.add_steps(Z_AXIS, ...)
extern void marlin_server_do_babystep_Z(float offs);

// direct call of 'enqueue_and_echo_command', returns 1 if command enqueued, otherwise 0
extern int marlin_server_enqueue_gcode(const char *gcode);

// direct call of 'inject_P', returns 1 if command enqueued, otherwise 0
extern int marlin_server_inject_gcode(const char *gcode);

// direct call of settings.save()
extern void marlin_server_settings_save(void);

// direct call of settings.load()
extern void marlin_server_settings_load(void);

// direct call of settings.reset()
extern void marlin_server_settings_reset(void);

// direct call of thermalManager.manage_heater()
extern void marlin_server_manage_heater(void);

// direct call of planner.quick_stop()
extern void marlin_server_quick_stop(void);

//
extern void marlin_server_print_abort(void);

//
extern void marlin_server_print_pause(void);

//
extern void marlin_server_print_resume(void);

//
extern void marlin_server_park_head(void);

//
extern int marlin_all_axes_homed(void);

//
extern int marlin_all_axes_known(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_MARLIN_SERVER_H
