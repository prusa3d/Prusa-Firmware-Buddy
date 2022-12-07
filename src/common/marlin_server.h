// marlin_server.h
#pragma once

#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "marlin_events.h"
#include "marlin_vars.h"
#include "marlin_errors.h"
#include "client_fsm_types.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

// server flags
// FIXME define the same type for these and marlin_server.flags
static const uint16_t MARLIN_SFLG_STARTED = 0x0001; // server started (set in marlin_server_init)
static const uint16_t MARLIN_SFLG_PROCESS = 0x0002; // loop processing in main thread is enabled
static const uint16_t MARLIN_SFLG_BUSY = 0x0004;    // loop is busy
static const uint16_t MARLIN_SFLG_PENDREQ = 0x0008; // pending request
static const uint16_t MARLIN_SFLG_EXCMODE = 0x0010; // exclusive mode enabled (currently used for selftest/wizard)

// server variable update interval [ms]
static const uint8_t MARLIN_UPDATE_PERIOD = 100;

typedef void(marlin_server_idle_t)(void);

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

// direct call of babystep.add_steps(Z_AXIS, ...)
extern void marlin_server_do_babystep_Z(float offs);

extern void marlin_server_move_axis(float pos, float feedrate, size_t axis);

// direct call of 'enqueue_and_echo_command'
// @retval true command enqueued
// @retval false otherwise
extern bool marlin_server_enqueue_gcode(const char *gcode);

// direct call of 'inject_P'
// @retval true command enqueued
// @retval false otherwise
extern bool marlin_server_inject_gcode(const char *gcode);

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

// direct print file with SFM format
void marlin_server_print_start(const char *filename, bool skip_preview);

//
extern uint32_t marlin_server_get_command(void);

//
extern void marlin_server_set_command(uint32_t command);

//
extern void marlin_server_test_start(uint64_t mask);

//
extern void marlin_server_test_abort(void);

//
extern void marlin_server_print_abort(void);

//
extern void marlin_server_print_resume(void);

//
extern void marlin_server_print_reheat_start(void);

//
extern bool marlin_server_print_reheat_ready();

// return true if the printer is not moving (idle, paused, aborted or finished)
extern bool marlin_server_printer_idle();

typedef struct
{
    xyze_pos_t pos;    // resume position for unpark_head
    float nozzle_temp; // resume nozzle temperature
    uint8_t fan_speed; // resume fan speed
} resume_state_t;

//
extern void marlin_server_print_pause(void);

// return true if the printer is in the paused and not moving state
extern bool marlin_server_printer_paused();

// return the resume state during a paused print
extern resume_state_t *marlin_server_get_resume_data();

// set the resume state for unpausing a print
extern void marlin_server_set_resume_data(const resume_state_t *data);

/// Plans retract and returns E stepper position in mm
void marlin_server_retract();

/// Lifts printing head
void marlin_server_lift_head();

/// Parks head at print pause or crash
/// If Z lift or retraction wasn't performed
/// you can rerun them.
extern void marlin_server_park_head();

//
extern void marlin_server_unpark_head_XY(void);
extern void marlin_server_unpark_head_ZE(void);

//
extern int marlin_all_axes_homed(void);

//
extern int marlin_all_axes_known(void);

// returns state of exclusive mode (1/0)
extern int marlin_server_get_exclusive_mode(void);

// set state of exclusive mode (1/0)
extern void marlin_server_set_exclusive_mode(int exclusive);

// display different value than target, used in preheat
extern void marlin_server_set_temp_to_display(float value);

//
extern float marlin_server_get_temp_to_display(void);

//
extern float marlin_server_get_temp_nozzle(void);

//
extern void marlin_server_resuming_begin(void);

extern uint32_t marlin_server_get_user_click_count(void);

extern uint32_t marlin_server_get_user_move_count(void);

extern void marlin_server_nozzle_timeout_on();
extern void marlin_server_nozzle_timeout_off();

extern void marlin_server_forced_client_refresh();

#ifdef __cplusplus
}
#endif //__cplusplus
