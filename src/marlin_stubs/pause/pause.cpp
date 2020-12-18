/**
 * @file pause.cpp
 * @author Radek Vana
 * @brief stubbed version of marlin pause.cpp
 * mainly used for load / unload / change filament
 * @date 2020-12-18
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

#include "../../lib/Marlin/Marlin/src/lcd/extensible_ui/ui_api.h"
#include "../../lib/Marlin/Marlin/src/core/language.h"
#include "../../lib/Marlin/Marlin/src/lcd/ultralcd.h"

#include "../../lib/Marlin/Marlin/src/libs/nozzle.h"
#include "../../lib/Marlin/Marlin/src/feature/pause.h"

#include "pause_stubbed.hpp"
#include "marlin_server.hpp"
#include "filament_sensor.hpp"
#include "filament.h"
#include "client_response.hpp"
#include "RAII.hpp"
#include <cmath>

// private:
//check unsupported features
//filament sensor is no longer part of marlin thus it must be disabled
//HAS_BUZZER must be disabled, because we handle it differently than marlin
// clang-format off
#if (!ENABLED(EXTENSIBLE_UI)) || \
    (!ENABLED(ADVANCED_PAUSE_FEATURE)) || \
    HAS_FILAMENT_SENSOR || \
    HAS_BUZZER || \
    HAS_LCD_MENU || \
    NUM_RUNOUT_SENSORS > 1 || \
    ENABLED(DUAL_X_CARRIAGE) || \
    (!ENABLED(PREVENT_COLD_EXTRUSION)) || \
    ENABLED(ADVANCED_PAUSE_CONTINUOUS_PURGE) || \
    BOTH(FILAMENT_UNLOAD_ALL_EXTRUDERS, MIXING_EXTRUDER) || \
    ENABLED(SDSUPPORT)
#error unsupported
#endif
// clang-format on

PauseMenuResponse pause_menu_response;

//cannot be class member (externed in marlin)
uint8_t did_pause_print = 0;
fil_change_settings_t fc_settings[EXTRUDERS];

//cannot be class member (externed in marlin and used by M240 and tool_change)
void do_pause_e_move(const float &length, const feedRate_t &fr_mm_s) {
    current_position.e += length / planner.e_factor[active_extruder];
    line_to_current_position(fr_mm_s);
    planner.synchronize();
}

/*****************************************************************************/
//PrivatePhase

PrivatePhase::PrivatePhase()
    : phase(PhasesLoadUnload::_first)
    , load_unload_shared_phase(int(UnloadPhases_t::_init))
    , nozzle_restore_temp(NAN)
    , bed_restore_temp(NAN) {}

void PrivatePhase::setPhase(PhasesLoadUnload ph, uint8_t progress_tot) {
    phase = ph;
    fsm_change(ClientFSM::Load_unload, phase, progress_tot, progress_tot == 100 ? 100 : 0);
}

PhasesLoadUnload PrivatePhase::getPhase() const { return phase; }

Response PrivatePhase::getResponse() {
    const Response ret = ClientResponseHandler::GetResponseFromPhase(phase);
    //user just clicked
    if (ret != Response::_none) {
        restoreTemp();
    }
    return ret;
}

bool PrivatePhase::CanSafetyTimerExpire() const {
    if (hasTempToRestore())
        return false;                              // already expired
    return ClientResponses::HasButton(getPhase()); // button in current phase == can wait on user == can timeout
}

void PrivatePhase::NotifyExpiredFromSafetyTimer(float hotend_temp, float bed_temp) {
    if (CanSafetyTimerExpire()) {
        nozzle_restore_temp = hotend_temp;
        bed_restore_temp = bed_temp;
    }
}

void PrivatePhase::clrRestoreTemp() {
    nozzle_restore_temp = NAN;
    bed_restore_temp = NAN;
}

void PrivatePhase::restoreTemp() {
    if (!isnan(nozzle_restore_temp)) {
        thermalManager.setTargetHotend(nozzle_restore_temp, 0);
        nozzle_restore_temp = NAN;
    }
    if (!isnan(bed_restore_temp)) {
        thermalManager.setTargetBed(bed_restore_temp);
        bed_restore_temp = NAN;
    }
}

bool PrivatePhase::hasTempToRestore() const {
    return (!isnan(nozzle_restore_temp)) || (!isnan(bed_restore_temp));
}

/*****************************************************************************/
//Pause
Pause &Pause::Instance() {
    static Pause s;
    return s;
}

Pause::Pause()
    : unload_length(GetDefaultUnloadLength())
    , slow_load_length(GetDefaultSlowLoadLength())
    , fast_load_length(GetDefaultFastLoadLength())
    , purge_length(GetDefaultPurgeLength())
    , retract(GetDefaultRetractLength()) {
}

float Pause::GetDefaultFastLoadLength() {
    return fc_settings[active_extruder].load_length;
}

float Pause::GetDefaultSlowLoadLength() {
    return FILAMENT_CHANGE_SLOW_LOAD_LENGTH;
}

float Pause::GetDefaultUnloadLength() {
    return fc_settings[active_extruder].unload_length;
}

float Pause::GetDefaultPurgeLength() {
    return ADVANCED_PAUSE_PURGE_LENGTH;
}

float Pause::GetDefaultRetractLength() {
    return PAUSE_PARK_RETRACT_LENGTH;
}

void Pause::SetUnloadLength(float len) {
    unload_length = -std::abs(isnan(len) ? GetDefaultUnloadLength() : len); // it is negative value
}

void Pause::SetSlowLoadLength(float len) {
    slow_load_length = std::abs(isnan(len) ? GetDefaultSlowLoadLength() : len);
}

void Pause::SetFastLoadLength(float len) {
    fast_load_length = std::abs(isnan(len) ? GetDefaultFastLoadLength() : len);
}

void Pause::SetPurgeLength(float len) {
    purge_length = std::max(std::abs(isnan(len) ? GetDefaultPurgeLength() : len), (float)minimal_purge);
}

void Pause::SetRetractLength(float len) {
    retract = -std::abs(isnan(len) ? GetDefaultRetractLength() : len); // retract is negative
}

void Pause::SetParkPoint(const xyz_pos_t &park_point) {
    park_pos = park_point; //TODO check limits
}

void Pause::SetResumePoint(const xyze_pos_t &resume_point) {
    resume_pos = resume_point; //TODO check limits
}

bool Pause::is_target_temperature_safe() {
    if (!DEBUGGING(DRYRUN) && thermalManager.targetTooColdToExtrude(active_extruder)) {
        SERIAL_ECHO_MSG(MSG_ERR_HOTEND_TOO_COLD);
        return false;
    } else {
        return true;
    }
}

bool Pause::ensureSafeTemperatureNotifyProgress(uint8_t progress_min, uint8_t progress_max) {
    if (!is_target_temperature_safe()) {
        return false;
    }

    if (Temperature::degHotend(active_extruder) + heating_phase_min_hotend_diff > Temperature::degTargetHotend(active_extruder)) { //do not disturb user with heating dialog
        return true;
    }

    Notifier_TEMP_NOZ N(ClientFSM::Load_unload, getPhaseIndex(), Temperature::degHotend(active_extruder),
        Temperature::degTargetHotend(active_extruder), progress_min, progress_max);

    return thermalManager.wait_for_hotend(active_extruder);
}

void Pause::do_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max) {
    //Not sure if folowing code would not be better
    //const float actual_e = planner.get_axis_position_mm(E_AXIS);
    //Notifier_POS_E N(ClientFSM::Load_unload, getPhaseIndex(), actual_e, actual_e + length, progress_min,progress_max);
    const float actual_e = current_position.e;
    current_position.e += length / planner.e_factor[active_extruder];
    Notifier_POS_E N(ClientFSM::Load_unload, getPhaseIndex(), actual_e, current_position.e, progress_min, progress_max);
    line_to_current_position(fr_mm_s);
    planner.synchronize();
}

void Pause::plan_e_move(const float &length, const feedRate_t &fr_mm_s) {
    current_position.e += length / planner.e_factor[active_extruder];
    while (!planner.buffer_line(current_position, fr_mm_s, active_extruder)) {
        delay(50);
    }
}

void Pause::plan_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max) {
    const float actual_e = current_position.e;
    current_position.e += length / planner.e_factor[active_extruder];
    Notifier_POS_E N(ClientFSM::Load_unload, getPhaseIndex(), actual_e, current_position.e, progress_min, progress_max);
    while (!planner.buffer_line(current_position, fr_mm_s, active_extruder)) {
        delay(50);
    }
}

bool Pause::loadLoop() {
    bool ret = true;
    const float purge_ln = std::max(purge_length, minimal_purge);

    const Response response = getResponse();

    //transitions
    switch (getLoadPhase()) {
    case LoadPhases_t::_init:
    case LoadPhases_t::has_slow_load:
        if (slow_load_length > 0) {
            set(LoadPhases_t::check_filament_sensor_and_user_push__ask);
        } else {
            set(LoadPhases_t::wait_temp);
        }
        break;
    case LoadPhases_t::check_filament_sensor_and_user_push__ask:
        if (fs_get_state() == fsensor_t::NoFilament) {
            setPhase(PhasesLoadUnload::MakeSureInserted);
        } else {
            setPhase(PhasesLoadUnload::UserPush);
            if (response == Response::Continue) {
                set(LoadPhases_t::load_in_gear);
            }
        }
        break;
    case LoadPhases_t::load_in_gear: { //slow load
        AutoRestore<bool> CE(thermalManager.allow_cold_extrude);
        thermalManager.allow_cold_extrude = true;
        setPhase(PhasesLoadUnload::Inserting, 10);
        do_e_move_notify_progress(slow_load_length, FILAMENT_CHANGE_SLOW_LOAD_FEEDRATE, 10, 30); // TODO method without param using actual phase
        set_filament(filament_to_load);
    }
        set(LoadPhases_t::wait_temp);
        break;
    case LoadPhases_t::wait_temp:
        setPhase(PhasesLoadUnload::WaitingTemp, 30);
        if (ensureSafeTemperatureNotifyProgress(30, 50)) {
            set(LoadPhases_t::long_load);
        } else {
            set(LoadPhases_t::error_temp);
        }
        break;
    case LoadPhases_t::error_temp:
        ret = false;
        set(LoadPhases_t::_finish);
        break;
    case LoadPhases_t::has_long_load:
        if (fast_load_length) {
            set(LoadPhases_t::long_load);
        } else {
            set(LoadPhases_t::stand_alone_purge);
        }
        break;
    case LoadPhases_t::long_load:
        planner.settings.retract_acceleration = FILAMENT_CHANGE_FAST_LOAD_ACCEL;
        setPhase(PhasesLoadUnload::Loading, 50);
        do_e_move_notify_progress(fast_load_length, FILAMENT_CHANGE_FAST_LOAD_FEEDRATE, 50, 70);
        set(LoadPhases_t::purge);
        break;
    case LoadPhases_t::purge:
        // Extrude filament to get into hotend
        setPhase(PhasesLoadUnload::Purging, 70);
        do_e_move_notify_progress(purge_ln, ADVANCED_PAUSE_PURGE_FEEDRATE, 70, 99);
        setPhase(PhasesLoadUnload::IsColor, 99);
        set(LoadPhases_t::ask_is_color_correct);
        break;
    case LoadPhases_t::stand_alone_purge:
        // Extrude filament to get into hotend
        setPhase(PhasesLoadUnload::Purging, 70);
        do_e_move_notify_progress(purge_ln, ADVANCED_PAUSE_PURGE_FEEDRATE, 70, 99);
        setPhase(PhasesLoadUnload::IsColorPurge, 99);
        set(LoadPhases_t::ask_is_color_correct__stand_alone_purge);
        break;
    case LoadPhases_t::ask_is_color_correct: {
        if (response == Response::Purge_more) {
            set(LoadPhases_t::purge);
        }
        if (response == Response::Retry) {
            set(LoadPhases_t::eject);
        }
        if (response == Response::Continue) {
            set(LoadPhases_t::_finish);
        }
    } break;
    case LoadPhases_t::ask_is_color_correct__stand_alone_purge: {
        if (response == Response::Purge_more) {
            set(LoadPhases_t::stand_alone_purge);
        }
        if (response == Response::Continue) {
            set(LoadPhases_t::_finish);
        }
    } break;
    case LoadPhases_t::eject:
        setPhase(PhasesLoadUnload::Ejecting, 99);
        do_e_move_notify_progress(-slow_load_length - fast_load_length - purge_ln, FILAMENT_CHANGE_FAST_LOAD_FEEDRATE, 10, 99);
        set(LoadPhases_t::has_slow_load);
        break;
    default:
        set(LoadPhases_t::_finish);
    }

    idle(true); // idle loop to prevet wdt and manage heaters etc, true == do not shutdown steppers
    return ret;
}

bool Pause::FilamentLoad() {
    FSM_HolderLoadUnload H(*this, LoadUnloadMode::Unload);
    return filamentLoad();
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
bool Pause::filamentLoad() {

    // actual temperature does not matter, only target
    if (!is_target_temperature_safe())
        return false;

#if ENABLED(PID_EXTRUSION_SCALING)
    bool extrusionScalingEnabled = thermalManager.getExtrusionScalingEnabled();
    thermalManager.setExtrusionScalingEnabled(false);
#endif //ENABLED(PID_EXTRUSION_SCALING)

    AutoRestore<float> AR(planner.settings.retract_acceleration);
    set(LoadPhases_t::_init);

    bool ret = true;
    do {
        ret = loadLoop();
    } while (getLoadPhase() != LoadPhases_t::_finish);

#if ENABLED(PID_EXTRUSION_SCALING)
    thermalManager.setExtrusionScalingEnabled(extrusionScalingEnabled);
#endif //ENABLED(PID_EXTRUSION_SCALING)

    return ret;
}

void Pause::unloadLoop() {
    static const RamUnloadSeqItem ramUnloadSeq[] = FILAMENT_UNLOAD_RAMMING_SEQUENCE;
    decltype(RamUnloadSeqItem::e) ramUnloadLength = 0; //Sum of ramming distances starting from first retraction

    constexpr float mm_per_minute = 1 / 60.f;
    constexpr size_t ramUnloadSeqSize = sizeof(ramUnloadSeq) / sizeof(RamUnloadSeqItem);

    const Response response = getResponse();

    //transitions
    switch (getUnloadPhase()) {
    case UnloadPhases_t::_init:
    case UnloadPhases_t::ram_sequence:
        setPhase(PhasesLoadUnload::Ramming, 50);
        {
            bool counting = false;
            for (size_t i = 0; i < ramUnloadSeqSize; ++i) {
                plan_e_move(ramUnloadSeq[i].e, ramUnloadSeq[i].feedrate * mm_per_minute);
                if (ramUnloadSeq[i].e < 0)
                    counting = true;
                if (counting)
                    ramUnloadLength += ramUnloadSeq[i].e;
            }
        }
        set(UnloadPhases_t::unload);
        break;
    case UnloadPhases_t::unload: {
        const float saved_acceleration = planner.settings.retract_acceleration;
        planner.settings.retract_acceleration = FILAMENT_CHANGE_UNLOAD_ACCEL;

        planner.synchronize(); //do_pause_e_move(0, (FILAMENT_CHANGE_UNLOAD_FEEDRATE));//do previous moves, so Ramming text is visible

        // subtract the already performed extruder movement from the total unload length
        setPhase(PhasesLoadUnload::Unloading, 51);
        do_e_move_notify_progress((unload_length - ramUnloadLength), (FILAMENT_CHANGE_UNLOAD_FEEDRATE), 51, 99);

        planner.settings.retract_acceleration = saved_acceleration;

        set_filament(FILAMENT_NONE);
        setPhase(PhasesLoadUnload::IsFilamentUnloaded, 100);
        set(UnloadPhases_t::unloaded__ask);
    } break;
    case UnloadPhases_t::unloaded__ask: {
        if (response == Response::Yes) {
            set(UnloadPhases_t::_finish);
        }
        if (response == Response::No) {
            setPhase(PhasesLoadUnload::ManualUnload, 100);
            disable_e_stepper(active_extruder);
            set(UnloadPhases_t::manual_unload);
        }

    } break;
    case UnloadPhases_t::manual_unload: {
        if (response == Response::Continue) {
            enable_e_steppers();
            set(UnloadPhases_t::_finish);
        }
    } break;
    default:
        set(UnloadPhases_t::_finish);
    }

    idle(true);
}

bool Pause::FilamentUnload() {
    FSM_HolderLoadUnload H(*this, LoadUnloadMode::Load);
    return filamentUnload();
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
bool Pause::filamentUnload() {
    setPhase(PhasesLoadUnload::WaitingTemp);
    if (!ensureSafeTemperatureNotifyProgress(0, 50)) {
        return false;
    }

#if ENABLED(PID_EXTRUSION_SCALING)
    bool extrusionScalingEnabled = thermalManager.getExtrusionScalingEnabled();
    thermalManager.setExtrusionScalingEnabled(false);
#endif //ENABLED(PID_EXTRUSION_SCALING)

    set(UnloadPhases_t::_init);

    do {
        unloadLoop();
    } while (getUnloadPhase() != UnloadPhases_t::_finish);

#if ENABLED(PID_EXTRUSION_SCALING)
    thermalManager.setExtrusionScalingEnabled(extrusionScalingEnabled);
#endif //ENABLED(PID_EXTRUSION_SCALING)

    return true;
}

void Pause::park_nozzle_and_notify() {
    setPhase(PhasesLoadUnload::Parking);
    // Initial retract before move to filament change position
    if (retract && thermalManager.hotEnoughToExtrude(active_extruder))
        do_pause_e_move(retract, PAUSE_PARK_RETRACT_FEEDRATE);

    // Park the nozzle by moving up by z_lift and then moving to (x_pos, y_pos)
    if (!axes_need_homing()) {
        {
            const float target_Z = _MIN(park_pos.z, Z_MAX_POS);
            Notifier_POS_Z N(ClientFSM::Load_unload, getPhaseIndex(), current_position.z, target_Z, 0, Z_MOVE_PRECENT);
            do_blocking_move_to_z(target_Z, NOZZLE_PARK_Z_FEEDRATE);
        }
        {
            const bool x_greater_than_y = std::abs(current_position.x - park_pos.x) > std::abs(current_position.y - park_pos.y);
            const float &begin_pos = x_greater_than_y ? current_position.x : current_position.y;
            const float &end_pos = x_greater_than_y ? park_pos.x : park_pos.y;
            if (x_greater_than_y) {
                Notifier_POS_X N(ClientFSM::Load_unload, getPhaseIndex(), begin_pos, end_pos, Z_MOVE_PRECENT, 100);
                do_blocking_move_to_xy(park_pos, NOZZLE_PARK_XY_FEEDRATE);
            } else {
                Notifier_POS_Y N(ClientFSM::Load_unload, getPhaseIndex(), begin_pos, end_pos, Z_MOVE_PRECENT, 100);
                do_blocking_move_to_xy(park_pos, NOZZLE_PARK_XY_FEEDRATE);
            }
        }
        report_current_position();
    }
}

void Pause::unpark_nozzle_and_notify() {
    if (!axes_need_homing()) {
        setPhase(PhasesLoadUnload::Unparking);
        // Move XY to starting position, then Z
        {
            const bool x_greater_than_y = std::abs(current_position.x - resume_pos.x) > std::abs(current_position.y - resume_pos.y);
            const float &begin_pos = x_greater_than_y ? current_position.x : current_position.y;
            const float &end_pos = x_greater_than_y ? resume_pos.x : resume_pos.y;
            if (x_greater_than_y) {
                Notifier_POS_X N(ClientFSM::Load_unload, getPhaseIndex(), begin_pos, end_pos, 0, XY_MOVE_PRECENT);
                do_blocking_move_to_xy(resume_pos, NOZZLE_PARK_XY_FEEDRATE);
            } else {
                Notifier_POS_Y N(ClientFSM::Load_unload, getPhaseIndex(), begin_pos, end_pos, 0, XY_MOVE_PRECENT);
                do_blocking_move_to_xy(resume_pos, NOZZLE_PARK_XY_FEEDRATE);
            }
        }
        // Move Z_AXIS to saved position
        {
            Notifier_POS_Z N(ClientFSM::Load_unload, getPhaseIndex(), current_position.z, resume_pos.z, XY_MOVE_PRECENT, 100);
            do_blocking_move_to_z(resume_pos.z, feedRate_t(NOZZLE_PARK_Z_FEEDRATE));
        }
    }
}

/**
 * FilamentChange procedure
 *
 * - Abort if already paused
 * - Send host action for pause, if configured
 * - Abort if TARGET temperature is too low
 * - Display "wait for start of filament change" (if a length was specified)
 * - Initial retract, if current temperature is hot enough
 * - Park the nozzle at the given position
 * - Call FilamentUnload (if a length was specified)
 * - Load filament if specified, but only if:
 *   - a nozzle timed out, or
 *   - the nozzle is already heated.
 * - Display "wait for print to resume"
 * - Re-prime the nozzle...
 *   -  FWRETRACT: Recover/prime from the prior G10.
 * - Move the nozzle back to resume_position
 * - Sync the planner E to resume_position.e
 * - Send host action for resume, if configured
 * - Resume the current SD print job, if any
 */
void Pause::FilamentChange() {
    if (did_pause_print)
        return; // already paused

    if (!DEBUGGING(DRYRUN) && unload_length && thermalManager.targetTooColdToExtrude(active_extruder)) {
        SERIAL_ECHO_MSG(MSG_ERR_HOTEND_TOO_COLD);
        return; // unable to reach safe temperature
    }

    // Indicate that the printer is paused
    ++did_pause_print;

    print_job_timer.pause();

    // Wait for buffered blocks to complete
    planner.synchronize();

#if ENABLED(ADVANCED_PAUSE_FANS_PAUSE) && FAN_COUNT > 0
    thermalManager.set_fans_paused(true);
#endif

    {
        FSM_HolderLoadUnload H(*this, LoadUnloadMode::Change);

        //park_nozzle_and_notify(retract, park_pos);

        if (unload_length) // Unload the filament
            filamentUnload();

        filamentLoad();

        //unpark_nozzle_and_notify();
    }

// Intelligent resuming
#if ENABLED(FWRETRACT)
    // If retracted before goto pause
    if (fwretract.retracted[active_extruder])
        do_pause_e_move(-fwretract.settings.retract_length, fwretract.settings.retract_feedrate_mm_s);
#endif

#if ADVANCED_PAUSE_RESUME_PRIME != 0
    do_pause_e_move(ADVANCED_PAUSE_RESUME_PRIME, feedRate_t(ADVANCED_PAUSE_PURGE_FEEDRATE));
#endif

    // Now all extrusion positions are resumed and ready to be confirmed
    // Set extruder to saved position
    planner.set_e_position_mm((destination.e = current_position.e = resume_pos.e));

    --did_pause_print;

#if ENABLED(ADVANCED_PAUSE_FANS_PAUSE) && FAN_COUNT > 0
    thermalManager.set_fans_paused(false);
#endif

    // Resume the print job timer if it was running
    if (print_job_timer.isPaused())
        print_job_timer.start();

    fs_clr_sent(); //reset filament sensor M600 sent flag

#if HAS_DISPLAY
    ui.reset_status();
#endif
}
