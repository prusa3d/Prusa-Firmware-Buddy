// marlin_client.h
#pragma once

#include "marlin_events.h"
#include "marlin_vars.h"
#include "marlin_errors.h"
#include "client_fsm_types.h"

// client flags
static const uint16_t MARLIN_CFLG_STARTED = 0x01; // client started (set in marlin_client_init)
static const uint16_t MARLIN_CFLG_PROCESS = 0x02; // loop processing in main thread is enabled
static const uint16_t MARLIN_CFLG_LOWHIGH = 0x08; // receiving low/high part of client message

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//-----------------------------------------------------------------------------
//externs from marlin server todo fixme use variables
extern int marlin_all_axes_homed(void);

extern int marlin_all_axes_known(void);

//-----------------------------------------------------------------------------
// client side functions (can be called from client thread only)

// initialize client side, returns pointer to client structure
extern marlin_vars_t *marlin_client_init(void);

// shutdown client (notimpl., TODO)
extern void marlin_client_shdn(void);

// client loop - must be called periodically in client thread
extern void marlin_client_loop(void);

// returns client_id for calling thread (-1 for unattached thread)
extern int marlin_client_id(void);

// infinite loop while server not ready
extern void marlin_client_wait_for_start_processing(void);

//sets dialog callback, returns 1 on success
extern int marlin_client_set_fsm_create_cb(fsm_create_t cb);
//sets dialog callback, returns 1 on success
extern int marlin_client_set_fsm_destroy_cb(fsm_destroy_t cb);
//sets dialog callback, returns 1 on success
extern int marlin_client_set_fsm_change_cb(fsm_change_t cb);
//sets dialog message, returns 1 on success
extern int marlin_client_set_message_cb(message_cb_t cb);
//sets dialog message, returns 1 on success
extern int marlin_client_set_warning_cb(warning_cb_t cb);
//sets startup callback, returns 1 on success
extern int marlin_client_set_startup_cb(startup_cb_t cb);
// returns enabled status of loop processing
extern int marlin_processing(void);

//sets event notification mask
extern void marlin_client_set_event_notify(uint64_t notify_events, void (*cb)());

//sets variable change notification mask
extern void marlin_client_set_change_notify(uint64_t notify_changes, void (*cb)());

// returns currently running command or MARLIN_CMD_NONE
extern uint32_t marlin_command(void);

// enable/disable exclusive mode (used for selftest)
extern void marlin_set_exclusive_mode(int exclusive_mode);

// start marlin loop processing in server thread (request '!start')
extern void marlin_start_processing(void);

// stop marlin loop processing in server thread (request '!stop')
extern void marlin_stop_processing(void);

// returns motion status of all axes (1: any axis is moving, 0: no motion)
extern int marlin_motion(void);

// synchronously wait for motion short timeout
extern int marlin_wait_motion(uint32_t timeout);

// enqueue gcode - thread-safe version  (request '!g xxx')
extern void marlin_gcode(const char *gcode);

// enqueue gcode from ethernet command (json parsed)
extern void marlin_json_gcode(const char *gcode);

// enqueue gcode - printf-like, returns number of chars printed
extern int marlin_gcode_printf(const char *format, ...);

// inject gcode - thread-safe version  (request '!ig xxx')
extern void marlin_gcode_push_front(const char *gcode);

// returns current event status for evt_id
extern int marlin_event(MARLIN_EVT_t evt_id);

// returns current event status for evt_id and set event
extern int marlin_event_set(MARLIN_EVT_t evt_id);

// returns current event status for evt_id and clear event
extern int marlin_event_clr(MARLIN_EVT_t evt_id);

// returns current event status for all events as 64bit mask
extern uint64_t marlin_events(void);

// returns current change status for var_id
extern int marlin_change(uint8_t var_id);

// returns current change status for var_id and set change
extern int marlin_change_set(uint8_t var_id);

// returns current change status for var_id and clear change
extern int marlin_change_clr(uint8_t var_id);

// returns current change status for all variables as 64bit mask
extern uint64_t marlin_changes(void);

// returns current error status for err_id
extern int marlin_error(uint8_t err_id);

// returns current error status for err_id and set error
extern int marlin_error_set(uint8_t err_id);

// returns current error status for err_id and clear error
extern int marlin_error_clr(uint8_t err_id);

// returns current error status for all errors as 64bit mask
extern uint64_t marlin_errors(void);

// returns variable value from client structure by var_id
extern variant8_t marlin_get_var(uint8_t var_id);

// request server to set variable, returns previous value or error (notimpl., TODO)
extern variant8_t marlin_set_var(uint8_t var_id, variant8_t val);

// returns variable structure pointer for calling thread
extern marlin_vars_t *marlin_vars(void);

// send request to update variables at server side and wait for change notification
extern marlin_vars_t *marlin_update_vars(uint64_t msk);

// returns number of commands in gcode queue
extern uint8_t marlin_get_gqueue(void);

// returns maximum number of commands in gcode queue
extern uint8_t marlin_get_gqueue_max(void);

// returns number of records in planner queue
extern uint8_t marlin_get_pqueue(void);

// returns maximum number of records in planner queue
extern uint8_t marlin_get_pqueue_max(void);

// variable setters (internally calls marlin_set_var)
extern float marlin_set_target_nozzle(float val);
extern float marlin_set_display_nozzle(float val);
extern float marlin_set_target_bed(float val);
extern float marlin_set_z_offset(float val);
extern uint8_t marlin_set_fan_speed(uint8_t val);
extern uint16_t marlin_set_print_speed(uint16_t val);
extern uint16_t marlin_set_flow_factor(uint16_t val);
extern uint8_t marlin_set_wait_heat(uint8_t val);
extern uint8_t marlin_set_wait_user(uint8_t val);

extern void marlin_do_babysteps_Z(float offs);

extern void marlin_settings_save(void);

extern void marlin_settings_load(void);

extern void marlin_settings_reset(void);

extern void marlin_manage_heater(void);

extern void marlin_quick_stop(void);

extern void marlin_test_start(uint32_t mask);

extern void marlin_test_abort(void);

extern void marlin_print_start(const char *filename);

extern void marlin_print_abort(void);

extern void marlin_print_pause(void);

extern void marlin_print_resume(void);

extern void marlin_park_head(void);

extern void marlin_notify_server_about_encoder_move(void);

extern void marlin_notify_server_about_knob_click(void);

// returns 1 if reheating is in progress, otherwise 0
extern int marlin_reheating(void);

// radio button click
extern void marlin_encoded_response(uint32_t enc_phase_and_response);
#ifdef __cplusplus
}
#endif //__cplusplus
