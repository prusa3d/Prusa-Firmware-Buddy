#pragma once

#include "marlin_events.h"
#include "marlin_errors.h"
#include "client_fsm_types.h"
#include "marlin_vars.hpp"
#include "client_response.hpp"
#include <option/has_selftest.h>

namespace marlin_client {

// client flags
inline constexpr uint16_t MARLIN_CFLG_STARTED = 0x01; // client started (set in marlin_client_init)
inline constexpr uint16_t MARLIN_CFLG_PROCESS = 0x02; // loop processing in main thread is enabled
inline constexpr uint16_t MARLIN_CFLG_LOWHIGH = 0x08; // receiving low/high part of client message

//-----------------------------------------------------------------------------
// client side functions (can be called from client thread only)

// initialize client side, returns pointer to client structure
void init();

// client loop - must be called periodically in client thread
void loop();

// returns client_id for calling thread (-1 for unattached thread)
int get_id();

// infinite loop while server not ready
void wait_for_start_processing();

// sets dialog callback, returns true on success
bool set_fsm_cb(fsm_cb_t cb);
// sets dialog message, returns true on success
bool set_message_cb(message_cb_t cb);

// sets dialog message, returns true on success
bool set_warning_cb(warning_cb_t cb);
// sets startup callback, returns true on success
bool set_startup_cb(startup_cb_t cb);
// returns enabled status of loop processing
bool is_processing();

// sets event notification mask
void set_event_notify(uint64_t notify_events, void (*cb)());

// returns currently running command or marlin_server::Cmd::NONE
marlin_server::Cmd get_command();

// enqueue gcode - thread-safe version  (request '!g xxx')
void gcode(const char *gcode);

// enqueue gcode - printf-like, returns number of chars printed
int gcode_printf(const char *format, ...);

// inject gcode - thread-safe version  (request '!ig xxx')
void gcode_push_front(const char *gcode);

// returns current event status for evt_id
int event(marlin_server::Event evt_id);

// returns current event status for evt_id and clear event
int event_clr(marlin_server::Event evt_id);

// returns current event status for all events as 64bit mask
uint64_t events();

// returns current error status for err_id
int error(uint8_t err_id);

// returns current error status for err_id and set error
int error_set(uint8_t err_id);

// returns current error status for err_id and clear error
int error_clr(uint8_t err_id);

// returns current error status for all errors as 64bit mask
uint64_t errors();

// returns number of commands in gcode queue
uint8_t get_gqueue();

// returns number of records in planner queue
uint8_t get_pqueue();

// variable setters
void set_target_nozzle(float val, uint8_t hotend = marlin_server::CURRENT_TOOL);
void set_display_nozzle(float val, uint8_t hotend = marlin_server::CURRENT_TOOL);
void set_target_bed(float val);
void set_fan_speed(uint8_t val);
void set_print_speed(uint16_t val);
void set_flow_factor(uint16_t val, uint8_t hotend = marlin_server::CURRENT_TOOL);
void set_z_offset(float val);
void set_fan_check(bool val);
void set_fs_autoload(bool val);

void do_babysteps_Z(float offs);

#if ENABLED(CANCEL_OBJECTS)
/**
 * @brief Cancels object with given ID. Preferred over using a GCode to get immediate write-through without having to wait for current gcode (or all in queue) to finish (GCode queue size is limited, so it's better to go around it if it makes sense).
 *
 * @param object_id
 */
void cancel_object(int object_id);

/**
 * @brief Uncancels object with given ID. See cancel_object fnc why this is preferred over a gcode cancellation.
 *
 * @param object_id
 */
void uncancel_object(int object_id);

/**
 * @brief Cancel currently printed object. See cancel_object fnc why this is preferred over a gcode cancellation.
 *
 * @param object_id
 */
void cancel_current_object();

#endif

/**
 * @brief Move axis to a logical position.
 * @param logical_pos requested axis position in [mm] (logical coordinates)
 * @param feedrate requested feedrate in [mm/min]
 * @param axis axis number (0=X, 1=Y, 2=Z, 3=E)
 */
void move_axis(float logical_pos, float feedrate, uint8_t axis);

void settings_save();

void settings_load();

void settings_reset();

#if HAS_SELFTEST()
void test_start_for_tools(const uint64_t test_mask, const uint8_t tool_mask);
void test_start(const uint64_t test_mask);
void test_abort();
#endif

void print_start(const char *filename, marlin_server::PreviewSkipIfAble skip_preview);

// Should only be called after calling marlin_print_start with skip_preview = true
// to see if it really started. Calling it after a call to marlin_print_start with
// skip_preview = false will cause an infinit loop!
bool is_print_started();
// Should only be called after a call to marlin_print_exit to wait for it to take
// effect and check for success.
bool is_print_exited();

void marlin_gui_ready_to_print();
void marlin_gui_cant_print();

void print_abort();

void print_exit(); // close fsm

void print_pause();

void print_resume();

void park_head();

void notify_server_about_encoder_move();

void notify_server_about_knob_click();

// returns true if printer is printing, else false;
bool is_printing();

bool is_idle();

// returns true if reheating is in progress, otherwise 0
bool is_reheating();

// radio button click
void encoded_response(uint32_t enc_phase_and_response);

//-----------------------------------------------------------------------------
// client side functions (can be called from client thread only)

// returns if response send succeeded
// called in client finite state machine
template <class T>
bool FSM_response(T phase, Response response) {
    uint32_t encoded = ClientResponses::Encode(phase, response);
    if (encoded == uint32_t(-1)) {
        return false;
    }

    encoded_response(encoded);
    return true;
}

} // namespace marlin_client
