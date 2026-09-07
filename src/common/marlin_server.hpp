// marlin_server.hpp
#pragma once

#include <optional>
#include <atomic>
#include "marlin_vars.hpp"

#include "encoded_fsm_response.hpp"
#include <warning_type.hpp>
#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "marlin_events.h"
#include "client_fsm_types.hpp"
#include "marlin_server_extended_fsm_data.hpp"
#include <stddef.h>
#include <gcode/inject_queue_actions.hpp>
#include <marlin_server_shared.h>
#include <marlin_server_request.hpp>
#include <utils/publisher.hpp>
#include <tool_index.hpp>
#include <utils/storage/strong_index_array.hpp>
#include <option/has_indx.h>

#include <serial_printing.hpp>
#include <bsod/bsod.h>

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

// direct call of 'enqueue_and_echo_command'
void enqueue_gcode(const char *gcode);

[[nodiscard]] bool enqueue_gcode_try(const char *gcode);

// direct call of 'enqueue_and_echo_command' with formatting
void __attribute__((format(__printf__, 1, 2)))
enqueue_gcode_printf(const char *gcode, ...);

// direct call of 'inject_action'
// @retval true command enqueued in inject queue
// @retval false otherwise
bool inject(InjectQueueRecord record);

/// Similar to inject, but attempts to execute the provided gcode immediately
/// Like, really immediately.
///
/// If deemned appropriate, it interrupts the currently executed gcode using the crash recovery mechanism,
/// otherwise falls back to standard inject() mechanics.
///
/// If the interrupt path is selected and the g-code gets interrupted,
/// the interrupting gcode gets executed AFTER RECOVERY REHOMING and REHEATING, but BEFORE UNPARKING.
/// The filament WILL BE RETRACTED BY THE RECOVERY MECHANISM.
/// Double-check the marlin_server resuming mechanism to validate that whatever you want to do, it fits with the sequence.
/// Also be mindful that the resuming mechanism will unpark to resume_pos, INCLUDING EXTRUDER POSITION
///
/// Currently intended for a quick reaction to filament runouts,
/// really think twice if you'd need to use it somewhere else.
///
/// !!! If the gcode DOES interrupt something, it will get executed BEFORE the final unparking
/// !!! ONLY USE IF YOU 100% KNOW WHAT YOU'RE DOING
void gcode_interrupt(GCodeLiteral gcode);

// direct call of settings.save()
void settings_save();

// direct call of settings.reset()
void settings_reset();

#if HAS_SERIAL_PRINT()
// Start serial print (issue when gcodes start comming via serial line)
void serial_print_start();

/// Finalize serial print (exit print state and clean up)
/// this is meant to be gracefull print finish, called when print finishes sucessfully.
void serial_print_finalize();
#endif

/**
 * @brief Direct print file with SFN format.
 * @param filename file to print
 * @param resume_pos position in the file to start from
 * @param skip_preview can be used to skip preview thumbnail or toolmapping screen
 */
void print_start(const char *filename, const GCodeReaderPosition &resume_pos, PreviewSkipIfAble skip_preview = PreviewSkipIfAble::no, ResetToolMapping reset_tool_mapping = ResetToolMapping::yes);

//
void set_command(uint32_t command);

//
void print_abort();

//
void print_resume();

// Quick stop to avoid harm to the user
void quick_stop();

// return true if the printer is not moving (idle, paused, aborted or finished)
bool printer_idle();

// returns true if printer is printing, else false;
bool is_printing();

/// \returns whether the server is currently processing or has queued any gcodes, motion and such
bool is_processing();

/**
 * @brief Know if any print preview state is active.
 * @return true if print preview is on
 */
bool print_preview();

struct resume_state_t {
    xyze_pos_t pos = {}; // resume position for unpark_head
    StrongIndexArray<int16_t, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> nozzle_temp; // target nozzle temperatures
    uint8_t fan_speed = 0; // resume fan speed
    uint16_t print_speed = 0; // resume printing speed
#if HAS_INDX()
    uint8_t active_tool = PhysicalToolIndex::count; // raw tool index; PhysicalToolIndex::count == MARLIN_NO_TOOL_PICKED means no tool
#endif
};

//
void print_pause();

// return true if the printer is currently aborting or already aborted the print
bool aborting_or_aborted();

// return true if the printer is currently finishing or already finished the print
bool finishing_or_finished();

// return true if the printer is in the paused and not moving state
bool printer_paused();

// Printer is paused, preparing for a pause or resuming from a pause.
bool printer_paused_extended();

// return the resume state during a paused print
resume_state_t *get_resume_data();

// set the resume state for unpausing a print
void set_resume_data(const resume_state_t *data);

#if HAS_NOZZLE_CLEANER()
void unpark_prime();
#endif
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

/// You probably dont want to call this function explicitly
namespace call_manually {
    // display different value than target, used in preheat
    // Called automatically from setTargetHotend since 6.6. Don't forget to add it back if you're cherry-picking to older branches!
    void set_temp_to_display(float value, PhysicalToolIndex extruder);
} // namespace call_manually

// called to set target bed (sets both marlin_vars and thermal_manager)
void set_target_bed(int16_t value);

bool get_media_inserted();

//
void resuming_begin();

// Thread-safe way to send request that don't require parameters
void send_request_flag(const RequestFlag request);

const GCodeReaderStreamRestoreInfo &stream_restore_info();

/// Returns media position of the currently executed gcode
uint32_t media_position();
void set_media_position(uint32_t set);

void print_quick_stop_powerpanic();

int32_t get_knob_position();

// user can stop waiting for heating/cooling by pressing a button
bool can_stop_wait_for_heatup();
void can_stop_wait_for_heatup(bool val);

/// If the phase matches currently recorded response, return it and consume it.
/// Otherwise, return std::monostate and do not consume it.
FSMResponseVariant get_response_variant_from_phase(FSMAndPhase fsm_and_phase, bool consume_response = true);

/// Sets a FSM response to be processed
void set_response(const EncodedFSMResponse &response);

/// Clears any pending response for the provided FSM
void clear_fsm_response(ClientFSM fsm);

/// If the phase matches currently recorded response, return it and consume it.
/// Otherwise, return Response::_none and do not consume it.
inline Response get_response_from_phase(FSMAndPhase fsm_and_phase, bool consume_response = true) {
    return get_response_variant_from_phase(fsm_and_phase, consume_response).value_or<Response>(Response::_none);
}

/// idles() for FSM response for the given phase and then \returns it
FSMResponseVariant wait_for_response_variant(FSMAndPhase fsm_and_phase, uint32_t timeout_ms = 0);

/// idles() for FSM response for the given phase and then \returns it
/// @returns Response::_none if got a reponse that is not of the Response enum (or on timeout)
inline Response wait_for_response(FSMAndPhase fsm_and_phase, uint32_t timeout_ms = 0) {
    return wait_for_response_variant(fsm_and_phase, timeout_ms).value_or<Response>(Response::_none);
}

/// Replacement for the FSM_Notifier. Hooked callbacks will get called in marlin idle()
extern Publisher<> idle_publisher;

void fsm_create(FSMAndPhase fsm_and_phase, fsm::PhaseData data = {});

void fsm_change(FSMAndPhase fsm_and_phase, fsm::PhaseData data = {});

void fsm_destroy(ClientFSM type);

/// @returns whether a FSM is active
bool is_fsm_active(ClientFSM type);

template <FSMExtendedDataSubclass DATA_TYPE>
void fsm_change_extended(FSMAndPhase fsm_and_phase, DATA_TYPE data) {
    FSMExtendedDataManager::store(data);
    // TODO Investigate if this hack is still needed since we have fsm::States::generation
    //  We use this ugly hack that we increment fsm_change_data[0] every time data changed, to force redraw of GUI
    static std::array<uint8_t, 4> fsm_change_data = { 0 };
    fsm_change_data[0]++;
    fsm_change(fsm_and_phase, fsm_change_data);
}

class FSM_Holder : public Uncopyable {
    ClientFSM fsm;
    bool do_destroy;

public:
    FSM_Holder(ClientFSM fsm)
        : fsm { fsm }
        // Only destroy the fsm at the end if the fsm was not active prior to the holder
        // This is to support FSM Holder nesting
        , do_destroy { fsm != ClientFSM::_none && !is_fsm_active(fsm) } {
    }

    FSM_Holder(FSMAndPhase fsm_and_phase, fsm::PhaseData data = fsm::PhaseData())
        : FSM_Holder(fsm_and_phase.fsm) {
        change(fsm_and_phase, data);
    }

    void change(FSMAndPhase fsm_and_phase, fsm::PhaseData data = fsm::PhaseData()) {
        debug_assert(fsm_and_phase.fsm == fsm);
        fsm_change(fsm_and_phase, data);
    }

    ~FSM_Holder() {
        if (do_destroy) {
            fsm_destroy(fsm);
        }
    }
};

void set_warning(WarningType type);
void clear_warning(WarningType type);
bool is_warning_active(WarningType type);

/// Displays a warning and blockingly waits for the response
Response prompt_warning(WarningType type, uint32_t timeout_ms = 0);

#if ENABLED(AXIS_MEASURE)
// Sets length of X and Y axes for crash recovery
void set_axes_length(xy_float_t xy);
#endif

void powerpanic_resume(const char *media_SFN_path, const GCodeReaderPosition &resume_pos, bool auto_recover);
void powerpanic_finish_recovery();
void powerpanic_finish_pause();
void powerpanic_finish_toolcrash();
#if HAS_INDX()
void powerpanic_finish_indx_toolchange();
#endif

void request_calibrations_screen();

/// @returns whether the current thread is the server thread
bool is_marlin_server_thread();

} // namespace marlin_server
