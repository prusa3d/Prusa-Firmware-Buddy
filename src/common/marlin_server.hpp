// marlin_server.hpp
#pragma once

#include <optional>
#include <atomic>
#include "marlin_vars.hpp"

#include "encoded_fsm_response.hpp"
#include <warning_type.hpp>
#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "marlin_events.h"
#include "client_fsm_types.h"
#include "marlin_server_extended_fsm_data.hpp"
#include <stddef.h>
#include <gcode/inject_queue_actions.hpp>

#include <serial_printing.hpp>

#if BOARD_IS_DWARF()
    #error "You're trying to add marlin_server to Dwarf. Don't!"
#endif /*BOARD_IS_DWARF()*/

/// Determines how full should the gcode queue be kept when fetching from media
/// You need at least one free slot for commands from serial (and UI)
#define MEDIA_FETCH_GCODE_QUEUE_FILL_TARGET (BUFSIZE - 1)

class GCodeReaderStreamRestoreInfo;
struct GCodeReaderPosition;

namespace marlin_server {

// server flags
// FIXME define the same type for these and marlin_server.flags
constexpr uint16_t MARLIN_SFLG_BUSY = 0x0004; // loop is busy
constexpr uint16_t MARLIN_SFLG_EXCMODE = 0x0010; // exclusive mode enabled (currently used for selftest/wizard)
constexpr uint16_t MARLIN_SFLG_STOPPED = 0x0020; // moves stopped until command drain

// server variable update interval [ms]
constexpr uint8_t MARLIN_UPDATE_PERIOD = 100;

//-----------------------------------------------------------------------------
// server side functions (can be called from server thread only)

// initialize server side - must be called at beginning in server thread
void init();

// server loop - must be called periodically in server thread
void loop();

// direct call of babystep.add_steps(Z_AXIS, ...)
void do_babystep_Z(float offs);

void move_axis(float pos, float feedrate, size_t axis);

// direct call of 'enqueue_and_echo_command'
// @retval true command enqueued
// @retval false otherwise
bool enqueue_gcode(const char *gcode);

// direct call of 'enqueue_and_echo_command' with formatting
// @retval true command enqueued
// @retval false otherwise
bool __attribute__((format(__printf__, 1, 2)))
enqueue_gcode_printf(const char *gcode, ...);

// direct call of 'inject_action'
// @retval true command enqueued in inject queue
// @retval false otherwise
bool inject(InjectQueueRecord record);

// direct call of settings.save()
void settings_save();

// direct call of settings.reset()
void settings_reset();

// Start serial print (issue when gcodes start comming via serial line)
void serial_print_start();

/**
 * @brief Direct print file with SFN format.
 * @param filename file to print
 * @param resume_pos position in the file to start from
 * @param skip_preview can be used to skip preview thumbnail or toolmapping screen
 */
void print_start(const char *filename, const GCodeReaderPosition &resume_pos, marlin_server::PreviewSkipIfAble skip_preview = marlin_server::PreviewSkipIfAble::no);

/// Finalize serial print (exit print state and clean up)
/// this is meant to be gracefull print finish, called when print finishes sucessfully.
void serial_print_finalize();

//
uint32_t get_command();

//
void set_command(uint32_t command);

//
void test_abort();

//
void print_abort();

//
void print_resume();

//
bool print_reheat_ready();

// Quick stop to avoid harm to the user
void quick_stop();

// Resume operation after quick_stop
void quick_resume();

// return true if the printer is not moving (idle, paused, aborted or finished)
bool printer_idle();

// returns true if printer is printing, else false;
bool is_printing();

/**
 * @brief Know if any print preview state is active.
 * @return true if print preview is on
 */
bool print_preview();

struct resume_state_t {
    xyze_pos_t pos = {}; // resume position for unpark_head
    float nozzle_temp[EXTRUDERS] = {}; // resume nozzle temperature
    bool nozzle_temp_paused = false; // True if nozzle_temp is valid and hotend cools down
    uint8_t fan_speed = 0; // resume fan speed
    uint16_t print_speed = 0; // resume printing speed
};

//
void print_pause();

void unpause_nozzle(const uint8_t extruder);

// return true if the printer is currently aborting or already aborted the print
bool aborting_or_aborted();

// return true if the printer is currently finishing or already finished the print
bool finishing_or_finished();

// return true if the printer is in the paused and not moving state
bool printer_paused();

// return the resume state during a paused print
resume_state_t *get_resume_data();

// set the resume state for unpausing a print
void set_resume_data(const resume_state_t *data);

/// Plans retract and returns E stepper position in mm
void retract();

/// Lifts printing head
void lift_head();

/// Parks head at print pause or crash
/// If Z lift or retraction wasn't performed
/// you can rerun them.
void park_head();

//
void unpark_head_XY();
void unpark_head_ZE();

//
bool all_axes_homed();

//
bool all_axes_known();

// returns state of exclusive mode (1/0)
int get_exclusive_mode();

// set state of exclusive mode (1/0)
void set_exclusive_mode(int exclusive);

// display different value than target, used in preheat
void set_temp_to_display(float value, uint8_t extruder);

// called to set target bed (sets both marlin_vars and thermal_manager)
void set_target_bed(float value);

bool get_media_inserted();

//
void resuming_begin();

const GCodeReaderStreamRestoreInfo &stream_restore_info();

/// Returns media position of the currently executed gcode
uint32_t media_position();
void set_media_position(uint32_t set);

void print_quick_stop_powerpanic();

uint32_t get_user_click_count();

uint32_t get_user_move_count();

void nozzle_timeout_on();
void nozzle_timeout_off();

class DisableNozzleTimeout {
public:
    DisableNozzleTimeout() {
        nozzle_timeout_off();
    }
    ~DisableNozzleTimeout() {
        nozzle_timeout_on();
    }
};

// user can stop waiting for heating/cooling by pressing a button
bool can_stop_wait_for_heatup();
void can_stop_wait_for_heatup(bool val);

/// If the phase matches currently recorded response, return it and consume it.
/// Otherwise, return std::monostate and do not consume it.
FSMResponseVariant get_response_variant_from_phase(FSMAndPhase fsm_and_phase);

/// If the phase matches currently recorded response, return it and consume it.
/// Otherwise, return Response::_none and do not consume it.
inline Response get_response_from_phase(FSMAndPhase fsm_and_phase) {
    return get_response_variant_from_phase(fsm_and_phase).value_or<Response>(Response::_none);
}

/// idles() for FSM response for the given phase and then \returns it
Response wait_for_response(FSMAndPhase fsm_and_phase);

// FSM_notifier
class FSM_notifier {
    struct data { // used floats - no need to retype
        ClientFSM type;
        uint8_t phase;
        float scale = 1; // scale from value to progress
        float offset = 0; // offset from lowest value
        uint8_t progress_min = 0;
        uint8_t progress_max = 100;
        const MarlinVariable<float> *var_id;
        std::optional<uint8_t> last_progress_sent;
        data()
            : type(ClientFSM::_none)
            , phase(0)
            , var_id(nullptr) {}
    };
    // static members
    // there can be only one active instance of FSM_notifier, which use this data
    static data s_data;
    static FSM_notifier *activeInstance;

    // temporary members
    // constructor stores previous state of FSM_notifier (its static data), destructor restores it
    data temp_data;

protected:
    FSM_notifier(const FSM_notifier &) = delete;

public:
    FSM_notifier(ClientFSM type, uint8_t phase, float min, float max, uint8_t progress_min, uint8_t progress_max, const MarlinVariable<float> &var_id);
    ~FSM_notifier();

    static void SendNotification();

    virtual fsm::PhaseData serialize(uint8_t progress) {
        return { { progress } };
    }
};

void fsm_create(FSMAndPhase fsm_and_phase, fsm::PhaseData data = {});

void fsm_change(FSMAndPhase fsm_and_phase, fsm::PhaseData data = {});

void fsm_destroy(ClientFSM type);

template <FSMExtendedDataSubclass DATA_TYPE>
void fsm_change_extended(FSMAndPhase fsm_and_phase, DATA_TYPE data) {
    FSMExtendedDataManager::store(data);
    // TODO Investigate if this hack is still needed since we have fsm::States::generation
    //  We use this ugly hack that we increment fsm_change_data[0] every time data changed, to force redraw of GUI
    static std::array<uint8_t, 4> fsm_change_data = { 0 };
    fsm_change_data[0]++;
    fsm_change(fsm_and_phase, fsm_change_data);
}

class FSM_Holder {
    ClientFSM type;

public:
    FSM_Holder(FSMAndPhase fsm_and_phase, fsm::PhaseData data = fsm::PhaseData())
        : type { fsm_and_phase.fsm } {
        fsm_create(fsm_and_phase, data);
    }

    ~FSM_Holder() {
        fsm_destroy(type);
    }
};

void set_warning(WarningType type);
void clear_warning(WarningType type);
bool is_warning_active(WarningType type);

/// Displays a warning and blockingly waits for the response
Response prompt_warning(WarningType type);

#if ENABLED(AXIS_MEASURE)
// Sets length of X and Y axes for crash recovery
void set_axes_length(xy_float_t xy);
#endif

void powerpanic_resume(const char *media_SFN_path, const GCodeReaderPosition &resume_pos, bool auto_recover);
void powerpanic_finish_recovery();
void powerpanic_finish_pause();
void powerpanic_finish_toolcrash();

void request_calibrations_screen();

} // namespace marlin_server
