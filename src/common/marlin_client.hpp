/**
 * @file marlin_client.hpp
 * @brief client side functions (can be called from client thread only)
 */
#pragma once
#include "marlin_events.h"
#include "marlin_vars.hpp"
#include "marlin_errors.h"
#include "client_fsm_types.h"
#include "client_response.hpp"

//-----------------------------------------------------------------------------
// externs from marlin server TODO FIXME use variables, or preferably remove
int marlin_all_axes_homed();
int marlin_all_axes_known();

namespace print_client {

// client flags
static constexpr uint16_t MARLIN_CFLG_STARTED = 0x01; // client started (set in init)
static constexpr uint16_t MARLIN_CFLG_PROCESS = 0x02; // loop processing in main thread is enabled
static constexpr uint16_t MARLIN_CFLG_LOWHIGH = 0x08; // receiving low/high part of client message

//-----------------------------------------------------------------------------
// client side functions (can be called from client thread only)

// initialize client side, returns pointer to client structure
marlin_vars_t *init(void);

// shutdown client (notimpl., TODO)
void shdn(void);

// client loop - must be called periodically in client thread
void loop(void);

// returns client_id for calling thread (-1 for unattached thread)
int id(void);

// infinite loop while server not ready
void wait_for_start_processing(void);

//sets dialog callback, returns 1 on success
int set_fsm_cb(fsm_cb_t cb);
//sets dialog message, returns 1 on success
int set_message_cb(message_cb_t cb);
//sets dialog message, returns 1 on success
int set_warning_cb(warning_cb_t cb);
//sets startup callback, returns 1 on success
int set_startup_cb(startup_cb_t cb);
// returns enabled status of loop processing
int processing(void);

//sets event notification mask
void set_event_notify(uint64_t notify_events, void (*cb)());

//sets variable change notification mask
void set_change_notify(uint64_t notify_changes, void (*cb)());

// returns currently running command or MARLIN_CMD_NONE
uint32_t command(void);

// enable/disable exclusive mode (used for selftest)
void set_exclusive_mode(int exclusive_mode);

// start marlin loop processing in server thread (request '!start')
void start_processing();

// stop marlin loop processing in server thread (request '!stop')
void stop_processing();

// returns motion status of all axes (1: any axis is moving, 0: no motion)
int motion();

// synchronously wait for motion short timeout
int wait_motion(uint32_t timeout);

// enqueue gcode - thread-safe version  (request '!g xxx')
void gcode(const char *gcode);

// enqueue gcode - printf-like, returns number of chars printed
int gcode_printf(const char *format, ...);

// inject gcode - thread-safe version  (request '!ig xxx')
void gcode_push_front(const char *gcode);

// returns current event status for evt_id
int event(MARLIN_EVT_t evt_id);

// returns current event status for evt_id and set event
int event_set(MARLIN_EVT_t evt_id);

// returns current event status for evt_id and clear event
int event_clr(MARLIN_EVT_t evt_id);

// returns current event status for all events as 64bit mask
uint64_t events();

// returns current change status for var_id
int change(marlin_var_id_t var_id);

// returns current change status for var_id and set change
int change_set(marlin_var_id_t var_id);

// returns current change status for var_id and clear change
int change_clr(marlin_var_id_t var_id);

// returns current change status for all variables as 64bit mask
uint64_t changes();

// returns current error status for err_id
int error(uint8_t err_id);

// returns current error status for err_id and set error
int error_set(uint8_t err_id);

// returns current error status for err_id and clear error
int error_clr(uint8_t err_id);

// returns current error status for all errors as 64bit mask
uint64_t errors();

// returns variable value from client structure by var_id
variant8_t get_var(marlin_var_id_t var_id);

float get_flt(marlin_var_id_t var_id);
uint32_t get_ui32(marlin_var_id_t var_id);
int32_t get_i32(marlin_var_id_t var_id);
uint16_t get_ui16(marlin_var_id_t var_id);
uint8_t get_ui8(marlin_var_id_t var_id);
int8_t get_i8(marlin_var_id_t var_id);
bool get_bool(marlin_var_id_t var_id);

// request server to set variable, returns previous value or error (notimpl., TODO)
variant8_t set_var(marlin_var_id_t var_id, variant8_t val);

void set_i8(marlin_var_id_t var_id, int8_t i8);
void set_bool(marlin_var_id_t var_id, bool b);
void set_ui8(marlin_var_id_t var_id, uint8_t ui8);
void set_i16(marlin_var_id_t var_id, int16_t i16);
void set_ui16(marlin_var_id_t var_id, uint16_t ui16);
void set_i32(marlin_var_id_t var_id, int32_t i32);
void set_ui32(marlin_var_id_t var_id, uint32_t ui32);
void set_flt(marlin_var_id_t var_id, float flt);

// returns variable structure pointer for calling thread
marlin_vars_t *vars();

// send request to update variables at server side and wait for change notification
marlin_vars_t *update_vars(uint64_t msk);

// returns number of commands in gcode queue
uint8_t get_gqueue();

// returns maximum number of commands in gcode queue
uint8_t get_gqueue_max();

// returns number of records in planner queue
uint8_t marlin_get_pqueue();

// returns maximum number of records in planner queue
uint8_t marlin_get_pqueue_max();

// variable setters (internally calls set_var)
float set_target_nozzle(float val);
float set_display_nozzle(float val);
float set_target_bed(float val);
uint8_t set_fan_speed(uint8_t val);
uint16_t set_print_speed(uint16_t val);
uint16_t set_flow_factor(uint16_t val);
bool set_wait_heat(bool val);
bool set_wait_user(bool val);

void do_babysteps_Z(float offs);

void move_axis(float pos, float feedrate, uint8_t axis);

void settings_save();

void settings_load();

void settings_reset();

void manage_heater();

void quick_stop();

void test_start(uint64_t mask);

void test_abort();

void print_start(const char *filename, bool skip_preview);

void gui_ready_to_print();

void print_abort();

void print_exit(); // close fsm

void print_pause();

void print_resume();

void park_head();

void notify_server_about_encoder_move();

void notify_server_about_knob_click();

//returns 1 if printer is printing, else 0;
bool is_printing();

// returns 1 if reheating is in progress, otherwise 0
int reheating();

// radio button click
void encoded_response(uint32_t enc_phase_and_response);

// returns if response send succeeded
// called in client finite state machine
template <class T>
bool FSM_response(T phase, Response response) {
    uint32_t encoded = ClientResponses::Encode(phase, response);
    if (encoded == uint32_t(-1))
        return false;

    encoded_response(encoded);
    return true;
}

}; // namespace print_client
