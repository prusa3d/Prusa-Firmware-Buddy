#pragma once

#include "marlin_events.h"
#include "marlin_server_types/client_fsm_types.h"
#include "encoded_fsm_response.hpp"
#include "marlin_vars.hpp"
#include "client_response.hpp"
#include <option/has_selftest.h>
#include "Marlin/src/core/types.h"
#include "common/selftest/selftest_data.hpp"
#include <gcode/inject_queue_actions.hpp>

namespace marlin_client {

//-----------------------------------------------------------------------------
// client side functions (can be called from client thread only)

/// Initialize the client side for the current task if it hasn't been initialized yet
/// Otherwise does nothing
void init_maybe();

// initialize client side, returns pointer to client structure
void init();

// client loop - must be called periodically in client thread
void loop();

// returns client_id for calling thread (-1 for unattached thread)
int get_id();

// sets dialog message, returns true on success
bool set_message_cb(message_cb_t cb);

// sets event notification mask
void set_event_notify(uint64_t notify_events);

// returns currently running command or marlin_server::Cmd::NONE
marlin_server::Cmd get_command();

// enqueue gcode - thread-safe version
void gcode(const char *gcode);

enum class GcodeTryResult {
    Submitted,
    QueueFull,
    GcodeTooLong,
};

// Like above, but may fail.
//
// May fail if the queue is currently full; doesn't go to redscreen or block indefinitely.
GcodeTryResult gcode_try(const char *gcode);

// enqueue gcode - printf-like
void __attribute__((format(__printf__, 1, 2)))
gcode_printf(const char *format, ...);

// inject gcode - thread-safe version
void inject(InjectQueueRecord record);

// inject gcode directly - thread-safe version
inline void inject(const char *gcode) { inject(GCodeLiteral(gcode)); };

// returns current event status for evt_id
int event(marlin_server::Event evt_id);

// returns current event status for evt_id and clear event
int event_clr(marlin_server::Event evt_id);

// returns current event status for all events as 64bit mask
uint64_t events();

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

#if HAS_SELFTEST()
void test_start_with_data(const uint64_t test_mask, const selftest::TestData test_data);
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

/// Tries to resume the print if it is in a problematic state
void try_recover_from_media_error();

void park_head();

void notify_server_about_encoder_move();

void notify_server_about_knob_click();

void set_warning(WarningType type);

/// If the specified warning is open, closes it
void clear_warning(WarningType type);

// returns true if printer is printing, else false;
bool is_printing();

bool is_paused();

bool is_idle();

//-----------------------------------------------------------------------------
// client side functions (can be called from client thread only)

void FSM_encoded_response(EncodedFSMResponse);

inline void FSM_response(FSMAndPhase fsm_and_phase, Response response) {
    FSM_encoded_response(EncodedFSMResponse { .response = FSMResponseVariant::make(response), .fsm_and_phase = fsm_and_phase });
}

inline void FSM_response_variant(FSMAndPhase fsm_and_phase, FSMResponseVariant response) {
    FSM_encoded_response(EncodedFSMResponse { .response = response, .fsm_and_phase = fsm_and_phase });
}

} // namespace marlin_client
