#pragma once
#include "marlin_events.h"
#include "marlin_errors.h"
#include "client_fsm_types.h"
#include "marlin_vars.hpp"
#include "client_response.hpp"
#include <option/has_selftest.h>

// client flags
inline constexpr uint16_t MARLIN_CFLG_STARTED = 0x01; // client started (set in marlin_client_init)
inline constexpr uint16_t MARLIN_CFLG_PROCESS = 0x02; // loop processing in main thread is enabled
inline constexpr uint16_t MARLIN_CFLG_LOWHIGH = 0x08; // receiving low/high part of client message

//-----------------------------------------------------------------------------
// client side functions (can be called from client thread only)

// initialize client side, returns pointer to client structure
void marlin_client_init();

// client loop - must be called periodically in client thread
void marlin_client_loop();

// returns client_id for calling thread (-1 for unattached thread)
int marlin_client_id();

// infinite loop while server not ready
void marlin_client_wait_for_start_processing();

// sets dialog callback, returns 1 on success
int marlin_client_set_fsm_cb(fsm_cb_t cb);
// sets dialog message, returns 1 on success
int marlin_client_set_message_cb(message_cb_t cb);

// sets dialog message, returns 1 on success
int marlin_client_set_warning_cb(warning_cb_t cb);
// sets startup callback, returns 1 on success
int marlin_client_set_startup_cb(startup_cb_t cb);
// returns enabled status of loop processing
int marlin_processing();

// sets event notification mask
void marlin_client_set_event_notify(uint64_t notify_events, void (*cb)());

// returns currently running command or MARLIN_CMD_NONE
uint32_t marlin_command();

// enqueue gcode - thread-safe version  (request '!g xxx')
void marlin_gcode(const char *gcode);

// enqueue gcode - printf-like, returns number of chars printed
int marlin_gcode_printf(const char *format, ...);

// inject gcode - thread-safe version  (request '!ig xxx')
void marlin_gcode_push_front(const char *gcode);

// returns current event status for evt_id
int marlin_event(marlin_server::Event evt_id);

// returns current event status for evt_id and clear event
int marlin_event_clr(marlin_server::Event evt_id);

// returns current event status for all events as 64bit mask
uint64_t marlin_events();

// returns current error status for err_id
int marlin_error(uint8_t err_id);

// returns current error status for err_id and set error
int marlin_error_set(uint8_t err_id);

// returns current error status for err_id and clear error
int marlin_error_clr(uint8_t err_id);

// returns current error status for all errors as 64bit mask
uint64_t marlin_errors();

// returns number of commands in gcode queue
uint8_t marlin_get_gqueue();

// returns number of records in planner queue
uint8_t marlin_get_pqueue();

// variable setters
void marlin_set_target_nozzle(float val, uint8_t hotend = marlin_server::CURRENT_TOOL);
void marlin_set_display_nozzle(float val, uint8_t hotend = marlin_server::CURRENT_TOOL);
void marlin_set_target_bed(float val);
void marlin_set_fan_speed(uint8_t val);
void marlin_set_print_speed(uint16_t val);
void marlin_set_flow_factor(uint16_t val, uint8_t hotend = marlin_server::CURRENT_TOOL);
void marlin_set_z_offset(float val);
void marlin_set_fan_check(bool val);
void marlin_set_fs_autoload(bool val);

void marlin_do_babysteps_Z(float offs);

/**
 * @brief Move axis to a logical position.
 * @param logical_pos requested axis position in [mm] (logical coordinates)
 * @param feedrate requested feedrate in [mm/min]
 * @param axis axis number (0=X, 1=Y, 2=Z, 3=E)
 */
void marlin_move_axis(float logical_pos, float feedrate, uint8_t axis);

void marlin_settings_save();

void marlin_settings_load();

void marlin_settings_reset();

#if HAS_SELFTEST()
void marlin_test_start_for_tools(const uint64_t test_mask, const uint8_t tool_mask);
void marlin_test_start(const uint64_t test_mask);
void marlin_test_abort();
#endif

void marlin_print_start(const char *filename, bool skip_preview);

// Should only be called after calling marlin_print_start with skip_preview = true
// to see if it really started. Calling it after a call to marlin_print_start with
// skip_preview = false will cause an infinit loop!
bool marlin_print_started();

void marlin_gui_ready_to_print();
void marlin_gui_cant_print();

void marlin_print_abort();

void marlin_print_exit(); // close fsm

void marlin_print_pause();

void marlin_print_resume();

void marlin_park_head();

void marlin_notify_server_about_encoder_move();

void marlin_notify_server_about_knob_click();

// returns true if printer is printing, else false;
bool marlin_is_printing();

// returns 1 if reheating is in progress, otherwise 0
int marlin_reheating();

// radio button click
void marlin_encoded_response(uint32_t enc_phase_and_response);

//-----------------------------------------------------------------------------
// client side functions (can be called from client thread only)

// returns if response send succeeded
// called in client finite state machine
template <class T>
bool marlin_FSM_response(T phase, Response response) {
    uint32_t encoded = ClientResponses::Encode(phase, response);
    if (encoded == uint32_t(-1))
        return false;

    marlin_encoded_response(encoded);
    return true;
}
