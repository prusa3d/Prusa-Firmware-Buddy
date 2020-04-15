/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * feature/pause.cpp - Pause feature support functions
 * This may be combined with related G-codes if features are consolidated.
 */
#include "../../lib/Marlin/Marlin/src/inc/MarlinConfigPre.h"

#include "../../lib/Marlin/Marlin/src/Marlin.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../lib/Marlin/Marlin/src/module/planner.h"
#include "../../lib/Marlin/Marlin/src/module/stepper.h"
#include "../../lib/Marlin/Marlin/src/module/printcounter.h"
#include "../../lib/Marlin/Marlin/src/module/temperature.h"

#if ENABLED(FWRETRACT)
    #include "fwretract.h"
#endif

#if ENABLED(HOST_ACTION_COMMANDS)
    #include "../../lib/Marlin/Marlin/src/feature/host_actions.h"
#endif

#include "../../lib/Marlin/Marlin/src/lcd/extensible_ui/ui_api.h"
#include "../../lib/Marlin/Marlin/src/core/language.h"
#include "../../lib/Marlin/Marlin/src/lcd/ultralcd.h"

#include "../../lib/Marlin/Marlin/src/libs/nozzle.h"
#include "../../lib/Marlin/Marlin/src/feature/pause.h"

#include "marlin_server.hpp"
// private:
//check unsupported features
//filament sensor is no longer part of marlin thus it must be disabled
// clang-format off
#if (!ENABLED(EXTENSIBLE_UI)) || \
    (!ENABLED(ADVANCED_PAUSE_FEATURE)) || \
    HAS_FILAMENT_SENSOR || \
    HAS_LCD_MENU || \
    NUM_RUNOUT_SENSORS > 1 || \
    ENABLED(DUAL_X_CARRIAGE) || \
    (!ENABLED(PREVENT_COLD_EXTRUSION)) || \
    ENABLED(ADVANCED_PAUSE_CONTINUOUS_PURGE) || \
    BOTH(FILAMENT_UNLOAD_ALL_EXTRUDERS, MIXING_EXTRUDER)
    #error unsupported
#endif
// clang-format on
static xyze_pos_t resume_position;

PauseMode pause_mode = PAUSE_MODE_PAUSE_PRINT;

PauseMenuResponse pause_menu_response;

fil_change_settings_t fc_settings[EXTRUDERS];

#if ENABLED(SDSUPPORT)
    #include "../../lib/Marlin/Marlin/src/sd/cardreader.h"
#endif

#if ENABLED(EMERGENCY_PARSER)
    #define _PMSG(L) L##_M108
#else
    #define _PMSG(L) L##_LCD
#endif

#if HAS_BUZZER
    #include "../../lib/Marlin/Marlin/src/libs/buzzer.h"
static void filament_change_beep(const int8_t max_beep_count, const bool init = false) {
    if (pause_mode == PAUSE_MODE_PAUSE_PRINT)
        return;
    static millis_t next_buzz = 0;
    static int8_t runout_beep = 0;

    if (init)
        next_buzz = runout_beep = 0;

    const millis_t ms = millis();
    if (ELAPSED(ms, next_buzz)) {
        if (max_beep_count < 0 || runout_beep < max_beep_count + 5) { // Only beep as long as we're supposed to
            next_buzz = ms + ((max_beep_count < 0 || runout_beep < max_beep_count) ? 1000 : 500);
            BUZZ(50, 880 - (runout_beep & 1) * 220);
            runout_beep++;
        }
    }
}
#else
static void filament_change_beep(const int8_t max_beep_count, const bool init = false) {}
#endif

/**
 * Ensure a safe temperature for extrusion
 *
 * - Fail if the TARGET temperature is too low
 * - Display LCD placard with temperature status
 * - Return when heating is done or aborted
 *
 * Returns 'true' if heating was completed, 'false' for abort
 */
static bool ensure_safe_temperature(const PauseMode mode = PAUSE_MODE_SAME) {

    if (!DEBUGGING(DRYRUN) && thermalManager.targetTooColdToExtrude(active_extruder)) {
        SERIAL_ECHO_MSG(MSG_ERR_HOTEND_TOO_COLD);
        return false;
    }

    UNUSED(mode);

    return thermalManager.wait_for_hotend(active_extruder);
}

static bool ensure_safe_temperature_notify_progress(PhasesLoadUnload phase, uint8_t progress_min, uint8_t progress_max) {

    if (!DEBUGGING(DRYRUN) && thermalManager.targetTooColdToExtrude(active_extruder)) {
        SERIAL_ECHO_MSG(MSG_ERR_HOTEND_TOO_COLD);
        return false;
    }

    Notifier_TEMP_NOZ N(ClinetFSM::Load_unload, GetPhaseIndex(phase),
        Temperature::degHotend(active_extruder), Temperature::degTargetHotend(active_extruder), progress_min, progress_max);

    return thermalManager.wait_for_hotend(active_extruder);
}

void do_pause_e_move(const float &length, const feedRate_t &fr_mm_s) {
    current_position.e += length / planner.e_factor[active_extruder];
    line_to_current_position(fr_mm_s);
    planner.synchronize();
}

void do_pause_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, PhasesLoadUnload phase, uint8_t progress_min, uint8_t progress_max) {
    //Not sure if folowing code would not be better
    //const float actual_e = planner.get_axis_position_mm(E_AXIS);
    //Notifier_POS_E N(ClinetFSM::Load_unload, GetPhaseIndex(phase), actual_e, actual_e + length, progress_min,progress_max);
    const float actual_e = current_position.e;
    current_position.e += length / planner.e_factor[active_extruder];
    Notifier_POS_E N(ClinetFSM::Load_unload, GetPhaseIndex(phase), actual_e, current_position.e, progress_min, progress_max);
    line_to_current_position(fr_mm_s);
    planner.synchronize();
}

void plan_pause_e_move(const float &length, const feedRate_t &fr_mm_s) {
    current_position.e += length / planner.e_factor[active_extruder];
    while (!planner.buffer_line(current_position, fr_mm_s, active_extruder)) {
        delay(50);
    }
}

void plan_pause_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, PhasesLoadUnload phase, uint8_t progress_min, uint8_t progress_max) {
    const float actual_e = current_position.e;
    current_position.e += length / planner.e_factor[active_extruder];
    Notifier_POS_E N(ClinetFSM::Load_unload, GetPhaseIndex(phase), actual_e, current_position.e, progress_min, progress_max);
    while (!planner.buffer_line(current_position, fr_mm_s, active_extruder)) {
        delay(50);
    }
}

/**
 * Load filament into the hotend
 *
 * - Fail if the a safe temperature was not reached
 * - If pausing for confirmation, wait for a click or M108
 * - Show "wait for load" placard
 * - Load and purge filament
 * - Show "Purge more" / "Continue" menu
 * - Return when "Continue" is selected
 *
 * Returns 'true' if load was completed, 'false' for abort
 */
bool load_filament(const float &slow_load_length /*=0*/, const float &fast_load_length /*=0*/, const float &purge_length /*=0*/, const int8_t max_beep_count /*=0*/,
    const bool show_lcd /*=false*/, const bool pause_for_user /*=false*/,
    const PauseMode mode /*=PAUSE_MODE_PAUSE_PRINT*/
        DXC_ARGS) {
    UNUSED(show_lcd);

    if (!ensure_safe_temperature_notify_progress(PhasesLoadUnload::WaitingTemp, 10, 30)) {
        return false;
    }

    fsm_change(ClinetFSM::Load_unload, GetPhaseIndex(PhasesLoadUnload::UserPush), 30, 0);
    while (ClientResponseHandler::GetResponseFromPhase(PhasesLoadUnload::UserPush) != Response::Continue)
        idle(true);

    //todo check FILAMET SENSOR

    // Slow Load filament
    if (slow_load_length) {
        do_pause_e_move_notify_progress(slow_load_length, FILAMENT_CHANGE_SLOW_LOAD_FEEDRATE, PhasesLoadUnload::Inserting, 30, 50);
    }

    // Fast Load Filament
    if (fast_load_length) {
        const float saved_acceleration = planner.settings.retract_acceleration;
        planner.settings.retract_acceleration = FILAMENT_CHANGE_FAST_LOAD_ACCEL;

        do_pause_e_move_notify_progress(fast_load_length, FILAMENT_CHANGE_FAST_LOAD_FEEDRATE, PhasesLoadUnload::Loading, 50, 70);

        planner.settings.retract_acceleration = saved_acceleration;
    }

    if (purge_length > 0) {
        Response response;
        do {
            // Extrude filament to get into hotend
            do_pause_e_move_notify_progress(purge_length, ADVANCED_PAUSE_PURGE_FEEDRATE, PhasesLoadUnload::Purging, 70, 99);
            fsm_change(ClinetFSM::Load_unload, GetPhaseIndex(PhasesLoadUnload::IsColor), 99, 0);
            do {
                idle();
                response = ClientResponseHandler::GetResponseFromPhase(PhasesLoadUnload::IsColor);
            } while (response == Response::_none);  //no button
        } while (response == Response::Purge_more); //purge more or continue .. exit loop
    }

    return true;
}

/**
 * Unload filament from the hotend
 *
 * - Fail if the a safe temperature was not reached
 * - Show "wait for unload" placard
 * - Retract, pause, then unload filament
 * - Disable E stepper (on most machines)
 *
 * Returns 'true' if unload was completed, 'false' for abort
 */
bool unload_filament(const float &unload_length, const bool show_lcd /*=false*/,
    const PauseMode mode /*=PAUSE_MODE_PAUSE_PRINT*/
) {
    UNUSED(show_lcd);

    if (!ensure_safe_temperature_notify_progress(PhasesLoadUnload::WaitingTemp, 10, 50)) {
        return false;
    }

    constexpr float mm_per_minute = 1 / 60.f;

    struct RamUnloadSeqItem {
        int16_t e;        ///< relative movement of Extruder
        int16_t feedrate; ///< feedrate of the move
    };

    static const RamUnloadSeqItem ramUnloadSeq[] = {
        { 1, 100 },
        { 1, 300 },
        { 3, 800 },
        { 2, 1200 },
        { 2, 2200 },
        { 2, 2600 }, // end of ramming
        { -2, 2200 },
        { -20, 3000 },
        { -30, 4000 }, // end of pre-unload
    };

    constexpr size_t pre_unload_begin_pos = 6;
    constexpr size_t ramUnloadSeqSize = sizeof(ramUnloadSeq) / sizeof(RamUnloadSeqItem);

    //cannot draw progress in plan_pause_e_move, so just change phase to ramming
    fsm_change(ClinetFSM::Load_unload, GetPhaseIndex(PhasesLoadUnload::Ramming), 50, 0);
    for (size_t i = 0; i < pre_unload_begin_pos; ++i) {
        plan_pause_e_move(ramUnloadSeq[i].e, ramUnloadSeq[i].feedrate * mm_per_minute);
    }

    for (size_t i = pre_unload_begin_pos; i < ramUnloadSeqSize; ++i) {
        plan_pause_e_move(ramUnloadSeq[i].e, ramUnloadSeq[i].feedrate * mm_per_minute); //cannot draw progress in plan_pause_e_move
    }

    // Unload filament
    const float saved_acceleration = planner.settings.retract_acceleration;
    planner.settings.retract_acceleration = FILAMENT_CHANGE_UNLOAD_ACCEL;

    planner.synchronize(); //do_pause_e_move(0, (FILAMENT_CHANGE_UNLOAD_FEEDRATE));//do previous moves, so Ramming text is visible

    // subtract the already performed extruder movement (-30mm) from the total unload length
    do_pause_e_move_notify_progress((unload_length + ramUnloadSeq[ramUnloadSeqSize - 1].e), (FILAMENT_CHANGE_UNLOAD_FEEDRATE), PhasesLoadUnload::Unloading, 51, 99);

    planner.settings.retract_acceleration = saved_acceleration;

// Disable E steppers for manual change
#if HAS_E_STEPPER_ENABLE
    disable_e_stepper(active_extruder);
    safe_delay(100);
#endif

    return true;
}

// public:

/**
 * Pause procedure
 *
 * - Abort if already paused
 * - Send host action for pause, if configured
 * - Abort if TARGET temperature is too low
 * - Display "wait for start of filament change" (if a length was specified)
 * - Initial retract, if current temperature is hot enough
 * - Park the nozzle at the given position
 * - Call unload_filament (if a length was specified)
 *
 * Return 'true' if pause was completed, 'false' for abort
 */
uint8_t did_pause_print = 0;

bool pause_print(const float &retract, const xyz_pos_t &park_point, const float &unload_length /*=0*/, const bool show_lcd /*=false*/ DXC_ARGS) {

    UNUSED(show_lcd);

    if (did_pause_print)
        return false; // already paused

#if ENABLED(HOST_ACTION_COMMANDS)
    #ifdef ACTION_ON_PAUSED
    host_action_paused();
    #elif defined(ACTION_ON_PAUSE)
    host_action_pause();
    #endif
#endif

#if ENABLED(HOST_PROMPT_SUPPORT)
    host_prompt_open(PROMPT_INFO, PSTR("Pause"), PSTR("Dismiss"));
#endif

    if (!DEBUGGING(DRYRUN) && unload_length && thermalManager.targetTooColdToExtrude(active_extruder)) {
        SERIAL_ECHO_MSG(MSG_ERR_HOTEND_TOO_COLD);

        return false; // unable to reach safe temperature
    }

    // Indicate that the printer is paused
    ++did_pause_print;

// Pause the print job and timer
#if ENABLED(SDSUPPORT)
    if (IS_SD_PRINTING()) {
        card.pauseSDPrint();
        ++did_pause_print; // Indicate SD pause also
    }
#endif

    print_job_timer.pause();

    // Save current position
    resume_position = current_position;

    // Wait for buffered blocks to complete
    planner.synchronize();

#if ENABLED(ADVANCED_PAUSE_FANS_PAUSE) && FAN_COUNT > 0
    thermalManager.set_fans_paused(true);
#endif

    // Initial retract before move to filament change position
    if (retract && thermalManager.hotEnoughToExtrude(active_extruder))
        do_pause_e_move(retract, PAUSE_PARK_RETRACT_FEEDRATE);

    // Park the nozzle by moving up by z_lift and then moving to (x_pos, y_pos)
    if (!axes_need_homing())
        nozzle.park(2, park_point);

    if (unload_length) // Unload the filament
        unload_filament(unload_length, show_lcd);

    return true;
}

/**
 * For Paused Print:
 * - Show "Press button (or M108) to resume"
 *
 * For Filament Change:
 * - Show "Insert filament and press button to continue"
 *
 * - Wait for a click before returning
 * - Heaters can time out and must reheat before continuing
 *
 * Used by M125 and M600
 */

void wait_for_confirmation(const bool is_reload /*=false*/, const int8_t max_beep_count /*=0*/ DXC_ARGS) {
    bool nozzle_timed_out = false;

    filament_change_beep(max_beep_count, true);

    // Start the heater idle timers
    const millis_t nozzle_timeout = (millis_t)(PAUSE_PARK_NOZZLE_TIMEOUT)*1000UL;

    HOTEND_LOOP()
    thermalManager.hotend_idle[e].start(nozzle_timeout);

    // Wait for filament insert by user and press button
    KEEPALIVE_STATE(PAUSED_FOR_USER);
    wait_for_user = true; // LCD click or M108 will clear this

    fsm_change(ClinetFSM::Load_unload, GetPhaseIndex(PhasesLoadUnload::UserPush), -1, 0);

    while (wait_for_user && (ClientResponseHandler::GetResponseFromPhase(PhasesLoadUnload::UserPush) != Response::Continue)) {
        filament_change_beep(max_beep_count);

        // If the nozzle has timed out...
        if (!nozzle_timed_out)
            HOTEND_LOOP()
        nozzle_timed_out |= thermalManager.hotend_idle[e].timed_out;

        // Wait for the user to press the button to re-heat the nozzle, then
        // re-heat the nozzle, re-show the continue prompt, restart idle timers, start over
        if (nozzle_timed_out) {
            SERIAL_ECHO_MSG(_PMSG(MSG_FILAMENT_CHANGE_HEAT));

            // Wait for LCD click or M108
            fsm_change(ClinetFSM::Load_unload, GetPhaseIndex(PhasesLoadUnload::NozzleTimeout), -1, 0);
            while (wait_for_user && (ClientResponseHandler::GetResponseFromPhase(PhasesLoadUnload::NozzleTimeout) != Response::Reheat))
                idle(true);

            // Re-enable the heaters if they timed out
            HOTEND_LOOP()
            thermalManager.reset_heater_idle_timer(e);

            // Wait for the heaters to reach the target temperatures
            ensure_safe_temperature();

            // Start the heater idle timers
            const millis_t nozzle_timeout = (millis_t)(PAUSE_PARK_NOZZLE_TIMEOUT)*1000UL;

            HOTEND_LOOP()
            thermalManager.hotend_idle[e].start(nozzle_timeout);

            wait_for_user = true;
            nozzle_timed_out = false;

            filament_change_beep(max_beep_count, true);
        }

        idle(true);
    }
}

/**
 * Resume or Start print procedure
 *
 * - If not paused, do nothing and return
 * - Reset heater idle timers
 * - Load filament if specified, but only if:
 *   - a nozzle timed out, or
 *   - the nozzle is already heated.
 * - Display "wait for print to resume"
 * - Re-prime the nozzle...
 *   -  FWRETRACT: Recover/prime from the prior G10.
 *   - !FWRETRACT: Retract by resume_position.e, if negative.
 *                 Not sure how this logic comes into use.
 * - Move the nozzle back to resume_position
 * - Sync the planner E to resume_position.e
 * - Send host action for resume, if configured
 * - Resume the current SD print job, if any
 */
void resume_print(const float &slow_load_length /*=0*/, const float &fast_load_length /*=0*/, const float &purge_length /*=ADVANCED_PAUSE_PURGE_LENGTH*/, const int8_t max_beep_count /*=0*/ DXC_ARGS) {
    /*
  SERIAL_ECHOLNPAIR(
    "start of resume_print()\ndual_x_carriage_mode:", dual_x_carriage_mode,
    "\nextruder_duplication_enabled:", extruder_duplication_enabled,
    "\nactive_extruder:", active_extruder,
    "\n"
  );
  //*/

    if (!did_pause_print)
        return;

    // Re-enable the heaters if they timed out
    bool nozzle_timed_out = false;
    HOTEND_LOOP() {
        nozzle_timed_out |= thermalManager.hotend_idle[e].timed_out;
        thermalManager.reset_heater_idle_timer(e);
    }

    if (nozzle_timed_out || thermalManager.hotEnoughToExtrude(active_extruder)) // Load the new filament
        load_filament(slow_load_length, fast_load_length, purge_length, max_beep_count, true, nozzle_timed_out, PAUSE_MODE_PAUSE_PRINT DXC_PASS);

// Intelligent resuming
#if ENABLED(FWRETRACT)
    // If retracted before goto pause
    if (fwretract.retracted[active_extruder])
        do_pause_e_move(-fwretract.settings.retract_length, fwretract.settings.retract_feedrate_mm_s);
#endif

    // If resume_position is negative
    if (resume_position.e < 0)
        do_pause_e_move(resume_position.e, feedRate_t(PAUSE_PARK_RETRACT_FEEDRATE));

    // Move XY to starting position, then Z
    do_blocking_move_to_xy(resume_position, feedRate_t(NOZZLE_PARK_XY_FEEDRATE));

    // Move Z_AXIS to saved position
    do_blocking_move_to_z(resume_position.z, feedRate_t(NOZZLE_PARK_Z_FEEDRATE));

#if ADVANCED_PAUSE_RESUME_PRIME != 0
    do_pause_e_move(ADVANCED_PAUSE_RESUME_PRIME, feedRate_t(ADVANCED_PAUSE_PURGE_FEEDRATE));
#endif

    // Now all extrusion positions are resumed and ready to be confirmed
    // Set extruder to saved position
    planner.set_e_position_mm((destination.e = current_position.e = resume_position.e));

#ifdef ACTION_ON_RESUMED
    host_action_resumed();
#elif defined(ACTION_ON_RESUME)
    host_action_resume();
#endif

    --did_pause_print;

#if ENABLED(HOST_PROMPT_SUPPORT)
    host_prompt_open(PROMPT_INFO, PSTR("Resuming"), PSTR("Dismiss"));
#endif

#if ENABLED(SDSUPPORT)
    if (did_pause_print) {
        card.startFileprint();
        --did_pause_print;
    }
#endif

#if ENABLED(ADVANCED_PAUSE_FANS_PAUSE) && FAN_COUNT > 0
    thermalManager.set_fans_paused(false);
#endif

    // Resume the print job timer if it was running
    if (print_job_timer.isPaused())
        print_job_timer.start();

#if HAS_DISPLAY
    ui.reset_status();
#endif
}
