#pragma once

#include "marlin_events.h"
#include "marlin_server_types/client_fsm_types.hpp"
#include "encoded_fsm_response.hpp"
#include <warning_type.hpp>
#include "marlin_vars.hpp"
#include "client_response.hpp"
#include <option/has_selftest.h>
#include "Marlin/src/core/types.h"
#include <option/has_selftest.h>
#if HAS_SELFTEST()
    #include "common/selftest/selftest_data.hpp"
#endif
#include "common/selftest/selftest_data.hpp"
#include <gcode/inject_queue_actions.hpp>
#include <tool_index.hpp>

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

// sets event notification mask
void set_event_notify(uint64_t notify_events);

// enqueue gcode - thread-safe version
//
// A gcode that doesn't fit into the request buffer is a programmer error and sends a warning to marlin server.
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

/// inject gcode - thread-safe version
///
/// @returns false if the record was dropped because the inject queue
// stayed full even after several retries.
bool inject(InjectQueueRecord record);

// inject gcode directly - thread-safe version
inline bool inject(ConstexprString gcode) { return inject(GCodeLiteral(gcode)); };

/// See marlin_server::gcode_interrupt for the documentation.
/// !!! ONLY USE IF YOU 100% KNOW WHAT YOU'RE DOING
void gcode_interrupt(GCodeLiteral gcode);

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

void set_target_nozzle(int16_t val, PhysicalToolIndex hotend);

void set_target_bed(int16_t val);
void set_fan_speed(uint8_t val);
void set_print_speed(uint16_t val);

void set_flow_factor(uint16_t val, VirtualToolIndex tool);
void set_z_offset(float val);
void set_fan_check(bool val);

void do_babysteps_Z(float offs);

#if HAS_CANCEL_OBJECT()
void set_object_cancelled(int object_id, bool set);

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

using PreviewSkipIfAble = marlin_server::PreviewSkipIfAble;
using ResetToolMapping = marlin_server::ResetToolMapping;

void print_start(const char *filename, PreviewSkipIfAble skip_preview = PreviewSkipIfAble::no, ResetToolMapping reset_tool_mapping = ResetToolMapping::yes);

// Should only be called after calling marlin_print_start with skip_preview = true
// to see if it really started. Calling it after a call to marlin_print_start with
// skip_preview = false will cause an infinit loop!
bool is_print_started();
// Should only be called after a call to marlin_print_exit to wait for it to take
// effect and check for success.
bool is_print_exited();

void print_abort();

void print_exit(); // close fsm

void print_pause();

void print_resume();

/// Tries to resume the print if it is in a problematic state
void try_recover_from_media_error();

void park_head();

void notify_server_about_encoder_move_up();

void notify_server_about_encoder_move_down();

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

void FSM_encoded_response(const EncodedFSMResponse &response);

inline void FSM_response(FSMAndPhase fsm_and_phase, Response response) {
    FSM_encoded_response(EncodedFSMResponse { .response = FSMResponseVariant::make(response), .fsm_and_phase = fsm_and_phase });
}

inline void FSM_response_variant(FSMAndPhase fsm_and_phase, FSMResponseVariant response) {
    FSM_encoded_response(EncodedFSMResponse { .response = response, .fsm_and_phase = fsm_and_phase });
}

} // namespace marlin_client
