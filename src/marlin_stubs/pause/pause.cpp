/**
 * @file pause.cpp
 * @author Radek Vana
 * @brief stubbed version of marlin pause.cpp
 * mainly used for load / unload / change filament
 * @date 2020-12-18
 */

#include "pause_stubbed.hpp"

#include "Marlin/src/Marlin.h"
#include "Marlin/src/gcode/gcode.h"
#include "Marlin/src/module/endstops.h"
#include "Marlin/src/module/motion.h"
#include "Marlin/src/module/planner.h"
#include "Marlin/src/module/stepper.h"
#include "Marlin/src/module/printcounter.h"
#include "Marlin/src/module/temperature.h"
#if ENABLED(PRUSA_MMU2)
    #include "Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#endif

#include "Marlin/src/lcd/extensible_ui/ui_api.h"
#include "Marlin/src/core/language.h"
#include "Marlin/src/lcd/ultralcd.h"

#if HAS_NOZZLE_CLEANER()
    #include <nozzle_cleaner.hpp>
    #include <gcode_loader.hpp>
#endif

#include "Marlin/src/feature/pause.h"
#include "filament_sensors_handler.hpp"
#include "marlin_server.hpp"
#include "fs_event_autolock.hpp"
#include "filament.hpp"
#include "client_response.hpp"
#include "fsm_loadunload_type.hpp"
#include "RAII.hpp"
#include "mapi/motion.hpp"
#include <cmath>
#include <logging/log.hpp>
#include <config_store/store_instance.hpp>
#include <scope_guard.hpp>
#include <filament_to_load.hpp>
#include <common/marlin_client.hpp>
#include <common/mapi/parking.hpp>
#include <feature/ramming/ramming_sequence.hpp>
#include <feature/ramming/standard_ramming_sequence.hpp>
#include <utils/progress.hpp>
#include <buddy/unreachable.hpp>
#include <feature/safety_timer/safety_timer.hpp>

#include <option/has_human_interactions.h>
#include <option/has_mmu2.h>
#include <option/has_wastebin.h>

#include <option/has_auto_retract.h>
#if HAS_AUTO_RETRACT()
    #include <feature/auto_retract/auto_retract.hpp>
#endif

LOG_COMPONENT_REF(MarlinServer);

#ifndef NOZZLE_UNPARK_XY_FEEDRATE
    #define NOZZLE_UNPARK_XY_FEEDRATE NOZZLE_PARK_XY_FEEDRATE
#endif

#if HAS_NOZZLE_CLEANER()
GCodeLoader nozzle_cleaner_gcode_loader;
#endif

// private:
// check unsupported features
// filament sensor is no longer part of marlin thus it must be disabled
// HAS_BUZZER must be disabled, because we handle it differently than marlin
// clang-format off
#if (!ENABLED(EXTENSIBLE_UI)) || \
    (!ENABLED(ADVANCED_PAUSE_FEATURE)) || \
    HAS_FILAMENT_SENSOR || \
    HAS_BUZZER || \
    NUM_RUNOUT_SENSORS > 1 || \
    ENABLED(DUAL_X_CARRIAGE) || \
    ENABLED(ADVANCED_PAUSE_CONTINUOUS_PURGE)
#error unsupported
#endif
// clang-format on

using namespace buddy;

static void report_progress(Pause &pause, ProgressPercent progress) {
    if (auto mode = pause.get_mode()) {
        const auto data = fsm::serialize_data(FSMLoadUnloadData { .mode = *mode, .progress = progress });
        marlin_server::fsm_change(pause.getPhase(), data);
    }
}

/// During its lifetime, automatically reports progress based on the current FSM state (thanks to ProgressMapper) and value of the specified marlin variable (hooks into marlin idle())
class PauseFsmNotifier : public CallbackHookGuard<> {

public:
    PauseFsmNotifier(Pause &pause, float min, float max, const MarlinVariable<float> &var)
        : CallbackHookGuard<>(marlin_server::idle_hook_point, [this, &var] {
            const auto progress = pause_.progress_mapper.update_progress(pause_.get_state(), to_normalized_progress(min_, max_, var.get()));
            report_progress(pause_, progress);
        })
        , pause_(pause)
        , min_(min)
        , max_(max) {}

private:
    Pause &pause_;
    float min_, max_;
};

/// Same as PauseFSMNotifier, but ties the progress to elapsed time instead of marlin variable
class PauseFsmDurationNotifier : public CallbackHookGuard<> {

public:
    PauseFsmDurationNotifier(Pause &pause, uint32_t duration_ms)
        : CallbackHookGuard<>(marlin_server::idle_hook_point, [this, duration_ms] {
            const auto progress = pause_.progress_mapper.update_progress(pause_.get_state(), to_normalized_progress(0, duration_ms, ticks_ms() - start_ms_));
            report_progress(pause_, progress);
        })
        , pause_(pause)
        , start_ms_(ticks_ms()) {}

private:
    Pause &pause_;
    uint32_t start_ms_;
};

// TODO Removeme; only for parking moves, which are not part of the actual FSM, so they cannot use the ProgressMapper
class PauseFsmExplicitProgressNotifier : public CallbackHookGuard<> {

public:
    PauseFsmExplicitProgressNotifier(Pause &pause, float min, float max, ProgressPercent progress_min, ProgressPercent progress_max, const MarlinVariable<float> &var)
        : CallbackHookGuard<>(marlin_server::idle_hook_point, [this, &var] {
            const auto progress = progress_span_.map(to_normalized_progress(min_, max_, var.get()));
            report_progress(pause_, progress);
        })
        , pause_(pause)
        , min_(min)
        , max_(max)
        , progress_span_(progress_min, progress_max) {}

private:
    Pause &pause_;
    float min_, max_;
    ProgressSpan progress_span_;
};

PauseMenuResponse pause_menu_response;

// cannot be class member (externed in marlin)
uint8_t did_pause_print = 0;

// cannot be class member (externed in marlin and used by M240 and tool_change)
void do_pause_e_move(const float &length, const feedRate_t &fr_mm_s) {
    mapi::extruder_move(length, fr_mm_s);
    planner.synchronize();
}

void unhomed_z_lift(float amount_mm) {
    if (amount_mm > current_position.z) {
        TemporaryGlobalEndstopsState park_move_endstops(true);
        do_homing_move((AxisEnum)(Z_AXIS), amount_mm - current_position.z, MMM_TO_MMS(HOMING_FEEDRATE_INVERTED_Z), false); // warning: the speed must probably be exactly this, otherwise endstops don't work
        // note: do_homing_move() resets the Marlin's internal position (Planner::position) to 0 (in Z axis) at the beginning
        current_position.z = amount_mm;
        sync_plan_position();
    }
}

#ifdef FILAMENT_RUNOUT_RAMMING_SEQUENCE
constexpr RammingSequenceArray runoutRammingSequence(FILAMENT_RUNOUT_RAMMING_SEQUENCE);
#else
constexpr RammingSequenceArray runoutRammingSequence(FILAMENT_UNLOAD_RAMMING_SEQUENCE);
#endif

constexpr RammingSequenceArray unloadRammingSequence(FILAMENT_UNLOAD_RAMMING_SEQUENCE);

/*****************************************************************************/
// PausePrivatePhase

PausePrivatePhase::PausePrivatePhase()
    : phase(PhasesLoadUnload::initial) {
}

void PausePrivatePhase::setPhase(PhasesLoadUnload ph) {
    phase = ph;
    if (load_unload_mode) {
        log_info(MarlinServer, "setPhase %i %i", int(ph), int(state));
        // Do not call progress_mapper.update_progress() here. We want to completely skip states that do nothing and remap the remaining progress
        // If we update progress here, the phase would be included in the progress mapping
        marlin_server::fsm_change(phase, fsm::serialize_data(FSMLoadUnloadData { .mode = *load_unload_mode, .progress = progress_mapper.current_progress() }));
    }
}

Response PausePrivatePhase::getResponse() {
    return marlin_server::get_response_from_phase(phase);
}

/*****************************************************************************/
// Pause
Pause &Pause::Instance() {
    static Pause s;
    return s;
}

bool Pause::is_unstoppable() const {
    switch (load_type) {
    case LoadType::load:
        return FSensors_instance().HasMMU();

    case LoadType::filament_change:
    case LoadType::filament_stuck:
        return true;

    case LoadType::autoload:
    case LoadType::load_purge:
    case LoadType::unload:
    case LoadType::unload_confirm:
    case LoadType::load_to_gears:
    case LoadType::unload_from_gears:
        return false;
    }

    bsod("Unhandled LoadType");
}

LoadUnloadMode Pause::get_load_unload_mode() {
    switch (load_type) {
    case Pause::LoadType::load:
    case Pause::LoadType::autoload:
    case Pause::LoadType::load_to_gears:
        return LoadUnloadMode::Load;
    case Pause::LoadType::load_purge:
        return LoadUnloadMode::Purge;
    case Pause::LoadType::unload:
    case Pause::LoadType::unload_confirm:
    case Pause::LoadType::unload_from_gears:
        return LoadUnloadMode::Unload;
    case Pause::LoadType::filament_change:
        return LoadUnloadMode::Change;
    case Pause::LoadType::filament_stuck:
        return LoadUnloadMode::FilamentStuck;
    }

    bsod("Unhandled LoadType");
}

bool Pause::should_park() {
    switch (load_type) {
    case Pause::LoadType::load_purge:
        return true;
    case Pause::LoadType::load_to_gears:
        return !FSensors_instance().has_filament_surely(LogicalFilamentSensor::extruder);
    case Pause::LoadType::autoload:
        // TODO: Change autoload trigger sensor on printers with side_fs
        // and adjust phases to handle properly loading to gears, mmu_rework and parking
        // autoload on printers with side_fs, should behave similary to iX autoload
    case Pause::LoadType::load:
        return option::has_human_interactions || !FSensors_instance().has_filament_surely(LogicalFilamentSensor::extruder);
    case Pause::LoadType::unload_from_gears:
        return false;
    default:
        return true;
    }
}

bool Pause::is_target_temperature_safe() {
    // Restore target temperatures, otherwise targetTooColdToExtrude would return true
    buddy::safety_timer().reset_restore_nonblocking();

#if HAS_AUTO_RETRACT()
    if (load_type == LoadType::unload && auto_retract().is_safely_retracted_for_unload(hotend_from_extruder(active_extruder))) {
        return true; // Its safe to unload even if the temp is too low if we are retracted
    }
#endif
    if (!DEBUGGING(DRYRUN) && thermalManager.targetTooColdToExtrude(active_extruder)) {
        SERIAL_ECHO_MSG(MSG_ERR_HOTEND_TOO_COLD);
        return false;
    } else {
        return true;
    }
}

bool Pause::ensureSafeTemperatureNotifyProgress() {
    // Do not disable heaters while we're waiting for the heatup, duh.
    // Also resets the safety timer, so it prevents it from triggering if we're periodically calling this
    buddy::SafetyTimerBlocker safety_timer_blocker;

    if (!is_target_temperature_safe()) {
        return false;
    }

    const auto is_tempreature_reached = [] {
        return
            // We're not restoring from a safety timer
            buddy::safety_timer().state() == buddy::SafetyTimer::State::idle

            // Pause-specific voodoo
            && Temperature::degHotend(active_extruder) + heating_phase_min_hotend_diff > Temperature::degTargetHotend(active_extruder);
    };

    if (is_tempreature_reached()) {
        // do not disturb user with heating dialog
        return true;
    }

    setPhase(is_unstoppable() ? PhasesLoadUnload::WaitingTemp_unstoppable : PhasesLoadUnload::WaitingTemp_stoppable);

    PauseFsmNotifier N(*this, Temperature::degHotend(active_extruder),
        Temperature::degTargetHotend(active_extruder) - heating_phase_min_hotend_diff, marlin_vars().hotend(active_extruder).temp_nozzle);

    // Wait until temperature is close
    while (!is_tempreature_reached()) {
        if (check_user_stop(getResponse())) {
            return false;
        }
        idle(true);
    }

    // Check that safety timer didn't disable heaters
    if (Temperature::degTargetHotend(active_extruder) == 0) {
        return false;
    }

    return true;
}

[[nodiscard]] Pause::StopConditions Pause::do_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, StopConditions check_for) {
    PauseFsmNotifier N(*this, current_position.e, current_position.e + length, marlin_vars().native_pos[MARLIN_VAR_INDEX_E]);

    mapi::extruder_move(length, fr_mm_s);

    return wait_for_motion_finish_stoppable(check_for);
}

[[nodiscard]] Pause::StopConditions Pause::do_e_move_notify_progress_coldextrude(const float &length, const feedRate_t &fr_mm_s, StopConditions check_for) {
    AutoRestore cold_extrude_guard(thermalManager.allow_cold_extrude, true);
    return do_e_move_notify_progress(length, fr_mm_s, check_for);
}

[[nodiscard]] Pause::StopConditions Pause::do_e_move_notify_progress_hotextrude(const float &length, const feedRate_t &fr_mm_s, StopConditions check_for) {
    PhasesLoadUnload last_ph = getPhase();

    if (!ensureSafeTemperatureNotifyProgress()) {
        return StopConditions::Failed;
    }

    // Restore phase after possible heatup GUI
    setPhase(last_ph);

    return do_e_move_notify_progress(length, fr_mm_s, check_for);
}

void Pause::plan_e_move(const float &length, const feedRate_t &fr_mm_s) {
    // It is unclear why this is one of the very few places where planner.buffer_line result is checked.
    // The draining condition is supposed to end the loop on power panic or crash.
    while (!mapi::extruder_move(length, fr_mm_s) && !planner.draining()) {
        delay(50);
    }
}

void Pause::start_process([[maybe_unused]] Response response) {
    setup_progress_mapper();

    switch (load_type) {
    case LoadType::load:
    case LoadType::autoload:
    case LoadType::load_to_gears:
    case LoadType::load_purge:
        set(LoadState::load_start);
        break;
    case LoadType::unload:
    case LoadType::unload_confirm:
    case LoadType::unload_from_gears:
    case LoadType::filament_change:
    case LoadType::filament_stuck:
        set(LoadState::unload_start);
        break;
    }
}

void Pause::load_start_process([[maybe_unused]] Response response) {
    // TODO: this shouldn't be needed here
    // actual temperature does not matter, only target
    if (!is_target_temperature_safe() && load_type != LoadType::load_to_gears && option::has_human_interactions) {
        set(LoadState::stop);
        return;
    }

#if HAS_MMU2()
    if (FSensors_instance().HasMMU()) {
        set(LoadState::mmu_load_start);
        return;
    }
#endif

    switch (load_type) {
    case LoadType::load_to_gears:
        // if extruder sensor is not working, we cannot load filament automatically, we need the user to manually confirm the the filament is pushed in
        if (!FSensors_instance().is_working(LogicalFilamentSensor::extruder)) {
            set(LoadState::filament_push_ask);
            break;
        }
        // If we are loading and filament is not in extruder = loading triggered by sideFS -> need asisting
        if (FSensors_instance().no_filament_surely(LogicalFilamentSensor::extruder)) {
            set_timed(LoadState::assist_insertion);
        } else {
            set(LoadState::load_to_gears);
        }
        break;

    case LoadType::autoload:
        // if filament is not present we want to break and not set loaded filament
        // we have already loaded the filament in gear, now just wait for temperature to rise
        config_store().set_filament_type(settings.GetExtruder(), filament::get_type_to_load());
        set(LoadState::load_wait_temp);
        handle_filament_removal(LoadState::filament_push_ask);
        break;
    case LoadType::load_purge:
        set(LoadState::load_wait_temp);
        break;
    default:
        if (option::has_side_fsensor && settings.extruder_mmu_rework) {
            if (FSensors_instance().has_filament_surely(LogicalFilamentSensor::extruder)) {
                set(LoadState::move_to_purge);
            } else {
                set_timed(LoadState::await_filament);
            }
        } else {
            set(LoadState::filament_push_ask);
        }
        break;
    }
}

void Pause::filament_push_ask_process(Response response) {
    if constexpr (!option::has_human_interactions) {
        setPhase(is_unstoppable() ? PhasesLoadUnload::Inserting_unstoppable : PhasesLoadUnload::Inserting_stoppable);

        if (FSensors_instance().has_filament_surely(LogicalFilamentSensor::extruder)) {
            set(LoadState::move_to_purge);
        } else if constexpr (option::has_side_fsensor) {
            set_timed(LoadState::await_filament);
        } else {
            set(LoadState::load_to_gears);
        }

        return;
    }

    if (FSensors_instance().no_filament_surely(LogicalFilamentSensor::extruder)) {
        setPhase(is_unstoppable() ? PhasesLoadUnload::MakeSureInserted_unstoppable : PhasesLoadUnload::MakeSureInserted_stoppable);
        handle_help(response);

        // With extruder MMU rework, we gotta assist the user with inserting the filament
        // BFW-5134
        if (settings.extruder_mmu_rework) {
#if ENABLED(PREVENT_COLD_EXTRUSION)
            AutoRestore ar_ce(thermalManager.allow_cold_extrude, true);
#endif
            mapi::extruder_schedule_turning(3);
        }

    } else {
        setPhase(is_unstoppable() ? PhasesLoadUnload::UserPush_unstoppable : PhasesLoadUnload::UserPush_stoppable);
        const bool has_filament = FSensors_instance().has_filament_surely(LogicalFilamentSensor::extruder);
        const bool is_mmu_rework_and_has_filament = settings.extruder_mmu_rework && has_filament;
        const bool side_fs_empty = FSensors_instance().no_filament_surely(LogicalFilamentSensor::side);
        const bool extruder_fs_not_working = !FSensors_instance().is_working(LogicalFilamentSensor::extruder);

        if (response == Response::Continue || is_mmu_rework_and_has_filament) {
            set(LoadState::load_to_gears);
        } else if (!is_unstoppable() && side_fs_empty && extruder_fs_not_working) { // We got to this state because extruder sensor is not working and side sensor triggered autoload (if now sideFS is empty we exit out)
            set(LoadState::stop);
        }
    }
}

void Pause::await_filament_process([[maybe_unused]] Response response) {
    setPhase(is_unstoppable() ? PhasesLoadUnload::AwaitingFilament_unstoppable : PhasesLoadUnload::AwaitingFilament_stoppable);
    // If EXTRUDER sensor is not assigned or not working, or if the user fails to insert filament in time, show Warning and quit loading.
    if (!FSensors_instance().is_working(LogicalFilamentSensor::extruder) || (!is_unstoppable() && ticks_diff(ticks_ms(), start_time_ms) > 10 * 60 * 1000)) {
        marlin_server::set_warning(WarningType::FilamentLoadingTimeout);
        set(LoadState::stop);
        return;
    }

    // Either side sensor not working or it has filament, go to loading
    if (!FSensors_instance().no_filament_surely(LogicalFilamentSensor::side)) {
        mapi::home_if_needed_and_park(mapi::ZAction::no_move, mapi::park_positions[mapi::ParkPosition::load]);
        if (settings.extruder_mmu_rework) {
            set_timed(LoadState::assist_insertion);
        } else {
            set(LoadState::filament_push_ask);
        }
        return;
    }
}

void Pause::runout_during_load_process([[maybe_unused]] Response response) {
    setPhase(PhasesLoadUnload::Ejecting_unstoppable);
    // unload immediately - we even cannot perform ramming as it would have consumed even more filament
    std::ignore = do_e_move_notify_progress_coldextrude(-std::abs(settings.unload_length), (FILAMENT_CHANGE_UNLOAD_FEEDRATE), StopConditions::Accomplished);

    // retry loading (similar to eject_process' final stages)
    switch (load_type) {
    case LoadType::load_to_gears:
    case LoadType::filament_change:
    case LoadType::filament_stuck:
        set(LoadState::load_start);
        break;
    case LoadType::load:
    case LoadType::autoload:
        set(LoadState::filament_push_ask);
        break;
    default:
        break;
    }
}

void Pause::assist_insertion_process([[maybe_unused]] Response response) {
    const bool unstoppable { is_unstoppable() };
    setPhase(unstoppable ? PhasesLoadUnload::Inserting_unstoppable : PhasesLoadUnload::Inserting_stoppable);

    // Filament is in Extruder autoload assistance is done.
    if (FSensors_instance().has_filament_surely(LogicalFilamentSensor::extruder)) {
        set(LoadState::load_to_gears);
        return;
    }

    // Load for at least 40 seconds before giving up.
    if (ticks_diff(ticks_ms(), start_time_ms) > 40000) { /*Move for at least 40 seconds before giving up*/
        /*
         * Unstoppable processes should not be stopped. Neither by user, nor printer on itself without any serious failure.
         * The branch used here ensures the printer remains in an infinite loop, waiting in an alert state until the filament is properly loaded—an expected behavior for the printer.
         * In all other cases, exiting the process does not harm the print. Instead, the user is notified that the filament change was not fully completed, and the printer resumes idling.
         */
        set(unstoppable ? LoadState::load_start : LoadState::stop);
        return;
    }

    // if filament is removed from side FS, stop too.
    if (FSensors_instance().no_filament_surely(LogicalFilamentSensor::side)) {
        set(LoadState::unload_finish_or_change);
        return;
    }

#if ENABLED(PREVENT_COLD_EXTRUSION)
    AutoRestore<bool> CE(thermalManager.allow_cold_extrude);
    thermalManager.allow_cold_extrude = true;
#endif
    // Enqueue an E move, but only if there are no more than 4 moves scheduled.
    // This ensures that there is always 0.4mm of movement enqueued in advance,
    // Guaranteeing a maximum movement difference of 0.1mm
    mapi::extruder_schedule_turning(FILAMENT_CHANGE_SLOW_LOAD_FEEDRATE, 0.1f);
}

void Pause::load_to_gears_process([[maybe_unused]] Response response) { // slow load
    setPhase(is_unstoppable() ? PhasesLoadUnload::LoadingToGears_unstoppable : PhasesLoadUnload::LoadingToGears_stoppable);

    const auto result = do_e_move_notify_progress_coldextrude(settings.slow_load_length, FILAMENT_CHANGE_SLOW_LOAD_FEEDRATE, StopConditions::All);

    if (result == StopConditions::SideFilamentSensorRunout) { // TODO method without param using actual phase
        set(LoadState::runout_during_load);
        return;
    }

    if (result == StopConditions::UserStopped) {
        set(LoadState::stop);
        return;
    }

    // if filament is not present we want to break and not set loaded filament
    if (load_type == LoadType::load_to_gears) {
        set(LoadState::_finished);
    } else {
        set(LoadState::move_to_purge);
    }
    handle_filament_removal(LoadState::filament_push_ask);
}

void Pause::move_to_purge_process([[maybe_unused]] Response response) {
    if constexpr (option::has_side_fsensor) {
        mapi::home_if_needed_and_park(mapi::ZAction::no_move, mapi::park_positions[mapi::ParkPosition::purge]);
    }
    set(LoadState::load_wait_temp);
}

void Pause::load_wait_temp_process([[maybe_unused]] Response response) {
    if (ensureSafeTemperatureNotifyProgress()) { // blocking -> checks for user stop
        if (load_type == LoadType::load_purge) {
            set(LoadState::purge);
        } else {
            set(LoadState::long_load);
        }
    }
    handle_filament_removal(LoadState::filament_push_ask);
}

void Pause::unload_wait_temp_process([[maybe_unused]] Response response) {
    if (!ensureSafeTemperatureNotifyProgress()) {
        return;
    }

    set(LoadState::ram_sequence);
}

void Pause::long_load_process([[maybe_unused]] Response response) {
    setPhase(is_unstoppable() ? PhasesLoadUnload::Loading_unstoppable : PhasesLoadUnload::Loading_stoppable);

    const float saved_acceleration = planner.user_settings.retract_acceleration;
    {
        auto s = planner.user_settings;
        s.retract_acceleration = FILAMENT_CHANGE_FAST_LOAD_ACCEL;
        planner.apply_settings(s);
    }

    auto move_e_progress = do_e_move_notify_progress_hotextrude(settings.fast_load_length, FILAMENT_CHANGE_FAST_LOAD_FEEDRATE, StopConditions::All);

    {
        auto s = planner.user_settings;
        s.retract_acceleration = saved_acceleration;
        planner.apply_settings(s);
    }

    if (move_e_progress == StopConditions::SideFilamentSensorRunout) {
        set(LoadState::runout_during_load);
        return;
    }

    set(LoadState::purge);
    handle_filament_removal(LoadState::filament_push_ask);
}

// Retract just a tiny bit to prevent oozing for a while.
// Note: current slicer default printer settings for ramming:
// MINI 2.5mm @ 70mm/s
// MK3 0.8mm @ 35mm/s
// MK4* 0.7mm @ 35mm/s
static constexpr float retract_distance = -4.f; // mm
static constexpr feedRate_t retract_feedrate = 35; // mm/s

void Pause::purge_process([[maybe_unused]] Response response) {
    // Extrude filament to get into hotend
    setPhase(is_unstoppable() ? PhasesLoadUnload::Purging_unstoppable : PhasesLoadUnload::Purging_stoppable);

    planner.synchronize(); // Finish any pending moves before starting the purge
    const auto purge_result = do_e_move_notify_progress_hotextrude(settings.purge_length(), ADVANCED_PAUSE_PURGE_FEEDRATE, StopConditions::All);
    if (purge_result == StopConditions::SideFilamentSensorRunout) {
        set(LoadState::runout_during_load);
        return;
    }
    // If the user stopped the purge, we need to stop the extruder move
    if (purge_result == StopConditions::UserStopped) {
        planner.quick_stop_and_resume();
    }
    // Skip retraction if Failed
    if (purge_result != StopConditions::Failed) {
        std::ignore = do_e_move_notify_progress_hotextrude(retract_distance, retract_feedrate, StopConditions::UserStopped);
    }

    config_store().set_filament_type(settings.GetExtruder(), filament::get_type_to_load());

    if constexpr (!option::has_human_interactions) {
        set(LoadState::load_prime);
        return;
    }

    setPhase(load_type == LoadType::load_purge ? PhasesLoadUnload::IsColorPurge : PhasesLoadUnload::IsColor);
    set(LoadState::color_correct_ask);
    handle_filament_removal(LoadState::filament_push_ask);
}

void Pause::color_correct_ask_process(Response response) {
    switch (response) {

    case Response::Purge_more:
        set(LoadState::purge);
        break;

    case Response::Retry:
        set(LoadState::eject);
        break;

    case Response::Yes:
        set(LoadState::load_prime);
        break;

    default:
        // This doesn't make sense with the MMU on
        if (!FSensors_instance().HasMMU()) {
            handle_filament_removal(LoadState::filament_push_ask);
        }
    }
}

#if HAS_MMU2()

void Pause::mmu_load_start_process([[maybe_unused]] Response response) {
    if (load_type == LoadType::load) {
        if (!MMU2::mmu2.load_filament_to_nozzle(settings.mmu_filament_to_load)) {
            // TODO tell user that he has already loaded filament if he really wants to continue
            // TODO check fsensor .. how should I behave if filament is not detected ???
            // some error?
            set(LoadState::load_prime);
            return;
        }

        config_store().set_filament_type(settings.GetExtruder(), filament::get_type_to_load());

        setPhase(PhasesLoadUnload::IsColor);
        set(LoadState::color_correct_ask);
    } else if (load_type == LoadType::filament_change) {
        if (settings.mmu_filament_to_load == MMU2::FILAMENT_UNKNOWN) {
            set(LoadState::load_prime);
            return;
        }

        setPhase(PhasesLoadUnload::LoadFilamentIntoMMU);
        set(LoadState::mmu_load_ask);
    }

    return;
}

void Pause::mmu_load_ask_process(Response response) {
    if (response == Response::Continue) {
        set(LoadState::mmu_load);
    }
}

void Pause::mmu_load_process([[maybe_unused]] Response response) {
    if (settings.mmu_filament_to_load == MMU2::FILAMENT_UNKNOWN) {
        set(LoadState::load_prime);
        return;
    }

    MMU2::mmu2.load_filament(settings.mmu_filament_to_load);
    MMU2::mmu2.load_filament_to_nozzle(settings.mmu_filament_to_load);

    setPhase(PhasesLoadUnload::IsColor);
    set(LoadState::color_correct_ask);
}

void Pause::mmu_unload_start_process([[maybe_unused]] Response response) {
    if (load_type == LoadType::unload) {
        MMU2::mmu2.unload();
        set(LoadState::_finished);
    } else if (load_type == LoadType::filament_change) {
        settings.mmu_filament_to_load = MMU2::mmu2.get_current_tool();

        // No filament loaded in MMU, we can't continue, as we don't know what slot to load
        if (settings.mmu_filament_to_load == MMU2::FILAMENT_UNKNOWN) {
            set(LoadState::unload_finish_or_change);
            return;
        }

        MMU2::mmu2.unload();
        MMU2::mmu2.eject_filament(settings.mmu_filament_to_load);
        set(LoadState::unload_finish_or_change);
    }

    return;
}
#endif

void Pause::eject_process([[maybe_unused]] Response response) {
#if HAS_MMU2()
    if (FSensors_instance().HasMMU()) {
        MMU2::mmu2.unload();
        if (load_type == LoadType::filament_change) {
            set(LoadState::mmu_load);
        } else {
            set(LoadState::load_start);
        }
        return;
    }
#endif

    if (!ram_filament()) {
        return; // Ramming unsuccessful (stopped by the user (button Stop) or temp not safe to extrude)
    }

    setPhase(is_unstoppable() ? PhasesLoadUnload::Ejecting_unstoppable : PhasesLoadUnload::Ejecting_stoppable);
    unload_filament();

    switch (load_type) {
    case LoadType::load_to_gears:
    case LoadType::filament_change:
    case LoadType::filament_stuck:
        set(LoadState::load_start);
        break;
    case LoadType::load:
    case LoadType::autoload:
        set(LoadState::filament_push_ask);
        break;
    default:
        break;
    }
}

void Pause::load_prime_process([[maybe_unused]] Response response) {
#if HAS_AUTO_RETRACT()
    if (!marlin_server::is_printing()) {
        // Only retract from nozzle outside printing
        set(LoadState::auto_retract);
        return;
    }
#endif

    if (load_type == LoadType::filament_change || load_type == LoadType::filament_stuck) {
        // Feed a little bit of filament to stabilize pressure in nozzle

        // Last poop after user clicked color - yes
        plan_e_move(std::abs(retract_distance), 10);

        // Retract again, it will be unretracted at the end of unpark
        if (settings.retract) {
            plan_e_move(settings.retract, PAUSE_PARK_RETRACT_FEEDRATE);
        }

        planner.synchronize();
        delay(500);
    }

#if HAS_NOZZLE_CLEANER()
    switch (load_type) {
    case LoadType::load:
    case LoadType::autoload:
    case LoadType::load_to_gears:
    case LoadType::load_purge:
        nozzle_cleaner_gcode_loader.load_gcode(nozzle_cleaner::load_filename, nozzle_cleaner::load_sequence);
        break;
    case LoadType::filament_change:
    case LoadType::filament_stuck:
        nozzle_cleaner_gcode_loader.load_gcode(nozzle_cleaner::runout_filename, nozzle_cleaner::runout_sequence);
        break;
    default:
        break;
    }

    set(LoadState::load_nozzle_clean);
    return;
#endif

    set(LoadState::_finished);
}

#if HAS_NOZZLE_CLEANER()
void Pause::load_nozzle_clean_process([[maybe_unused]] Response response) {
    setPhase(PhasesLoadUnload::LoadNozzleCleaning);
    auto loader_result = nozzle_cleaner_gcode_loader.get_result();

    if (loader_result.has_value()) {
        GcodeSuite::process_subcommands_now(loader_result.value());
    }

    if (loader_result.has_value() || loader_result.error() != GCodeLoader::BufferState::buffering) {
        nozzle_cleaner_gcode_loader.reset();
        set(LoadState::_finished);
    }
}
#endif

void Pause::stop_process([[maybe_unused]] Response response) {
    if (!planner.busy()) {
        // The printer is not moving, we don't need to do anything drastic and lose homing.
        set(LoadState::_stopped);
        return;
    }

    planner.quick_stop_and_resume();
    xyze_pos_t real_current_position;
    planner.get_axis_position_mm(static_cast<xyz_pos_t &>(real_current_position));
    real_current_position[E_AXIS] = 0;
#if HAS_POSITION_MODIFIERS
    planner.unapply_modifiers(real_current_position
    #if HAS_LEVELING
        ,
        true
    #endif
    );
#endif

    // Lose homing only if we interrupted XYZ movement. quick_stop on extruder movement is fine
    if (xyz_pos_t(current_position) != xyz_pos_t(real_current_position)) {
        set_all_unhomed();
    }

    current_position = real_current_position;
    planner.set_position_mm(current_position);

    set(LoadState::_stopped);
}

void Pause::unload_start_process([[maybe_unused]] Response response) {
    // loop_unload_mmu has it's own preheating sequence, use that one for better progress reporting
    if (!(load_type == LoadType::unload && FSensors_instance().HasMMU()) && !is_target_temperature_safe() && load_type != LoadType::unload_from_gears) {
        set(LoadState::stop);
        return;
    }

#if HAS_MMU2()
    if (FSensors_instance().HasMMU()) {
        set(LoadState::mmu_unload_start);
        return;
    }
#endif

    switch (load_type) {

    case LoadType::filament_stuck:
#if HAS_LOADCELL()
        set(LoadState::filament_stuck_ask);
#else
        set(LoadState::manual_unload);
#endif
        break;

    case LoadType::unload_from_gears:
        set(LoadState::unload_from_gears);
        break;

    default:
        set(LoadState::unload_wait_temp);
        break;
    }
}

#if HAS_LOADCELL()
void Pause::filament_stuck_ask_process(Response response) {
    setPhase(PhasesLoadUnload::FilamentStuck);

    if (response == Response::Unload) {
        set(LoadState::unload_wait_temp);
    }
}
#endif

#if HAS_AUTO_RETRACT()
void Pause::auto_retract_process([[maybe_unused]] Response response) {
    setPhase(PhasesLoadUnload::AutoRetracting);
    PauseFsmDurationNotifier progress_notifier(*this, standard_ramming_sequence(StandardRammingSequence::auto_retract, marlin_vars().active_hotend_id()).duration_estimate_ms());
    auto_retract().maybe_retract_from_nozzle();
    set(LoadState::_finished);
}
#endif

void Pause::ram_sequence_process([[maybe_unused]] Response response) {
#if HAS_AUTO_RETRACT()
    if (auto_retract().is_safely_retracted_for_unload()) {
        // The filament is already retracted from the nozzle -> no ramming needed, we don't even need to heat up the nozzle
        ram_retracted_distance = auto_retract().retracted_distance().value(); // We are sure value is not std::nullopt because of is_safely_retracted_for_unload()
        set(LoadState::unload);
        return;
    }
#endif

    if (ram_filament()) {
        set(LoadState::unload);
    }
}

void Pause::unload_process([[maybe_unused]] Response response) {
    setPhase(is_unstoppable() ? PhasesLoadUnload::Unloading_unstoppable : PhasesLoadUnload::Unloading_stoppable);
    unload_filament();

    config_store().set_filament_type(settings.GetExtruder(), FilamentType::none);

    switch (load_type) {
    case LoadType::unload:
        if constexpr (option::has_human_interactions) {
            set(LoadState::unload_finish_or_change);
            break;
        }
    case LoadType::unload_confirm:
    case LoadType::filament_change:
    case LoadType::filament_stuck:
#if HAS_NOZZLE_CLEANER()
        nozzle_cleaner_gcode_loader.load_gcode(nozzle_cleaner::unload_filename, nozzle_cleaner::unload_sequence);

        set(LoadState::unload_nozzle_clean);
        return;
#endif

        if constexpr (!option::has_human_interactions) {
            runout_timer_ms = ticks_ms();
            set(LoadState::filament_not_in_fs);
        } else {
            set(LoadState::unloaded_ask);
        }
        break;
    default:
        break;
    }
}

void Pause::unloaded_ask_process(Response response) {
    setPhase(PhasesLoadUnload::IsFilamentUnloaded);

    if (response == Response::Yes) {
        set(LoadState::filament_not_in_fs);
        return;
    }
    if (response == Response::No) {
        disable_e_stepper(active_extruder);
        set(LoadState::manual_unload);
    }
}

void Pause::unload_from_gears_process([[maybe_unused]] Response response) {
    setPhase(PhasesLoadUnload::Unloading_stoppable);

    // unload cannot cause a runout -> safe to ignore the result
    std::ignore = do_e_move_notify_progress_coldextrude(-settings.slow_load_length * (float)1.5, FILAMENT_CHANGE_FAST_LOAD_FEEDRATE, StopConditions::UserStopped);
    set(LoadState::unload_finish_or_change);
}

#if HAS_NOZZLE_CLEANER()
void Pause::unload_nozzle_clean_process([[maybe_unused]] Response response) {
    setPhase(PhasesLoadUnload::UnloadNozzleCleaning);
    auto loader_result = nozzle_cleaner_gcode_loader.get_result();

    if (loader_result.has_value()) {
        GcodeSuite::process_subcommands_now(loader_result.value());
    }

    if (loader_result.has_value() || loader_result.error() != GCodeLoader::BufferState::buffering) {
        nozzle_cleaner_gcode_loader.reset();
        if constexpr (!option::has_human_interactions) {
            runout_timer_ms = ticks_ms();
            set(LoadState::filament_not_in_fs);
        } else {
            set(LoadState::unloaded_ask);
        }
    }
}
#endif

void Pause::unload_finish_or_change_process([[maybe_unused]] Response response) {
    if (load_type == LoadType::filament_change || load_type == LoadType::filament_stuck) {
        set(LoadState::load_start);
    } else {
        set(LoadState::_finished);
    }
}

void Pause::filament_not_in_fs_process(Response response) {
    setPhase(PhasesLoadUnload::FilamentNotInFS);
    handle_help(response);
    // We want to use the sensor that is the closest to the extruder and will not be triggered by the printer itself
    // If we have mmu_rework, the unload itself ejects filament from sensor so we need to use the one that is further
    // Here the runout sensors are handy but in reverse order (since the primary is the further one from extruder)
    if (!FSensors_instance().has_filament_surely(settings.extruder_mmu_rework ? LogicalFilamentSensor::primary_runout : LogicalFilamentSensor::secondary_runout)) {
        if constexpr (!option::has_human_interactions) {
            // In case of no human interactions, require no filament being
            // detected for at least 1s to avoid FS flicking off and on due
            // to filament movement and reloading the just-unloaded
            // filament remnant back in.
            if (ticks_diff(ticks_ms(), runout_timer_ms) < 1000) {
                return;
            }
        }

        set(LoadState::unload_finish_or_change);
    } else {
        if constexpr (!option::has_human_interactions) {
            runout_timer_ms = ticks_ms();
        } else {
            // We do have human interaction but filament sensor
            // still detects filament after user confirmed unloading filament
            // As we do not want to enter an endless loop, ask again for confirmation
            set(LoadState::unloaded_ask);
        }
    }
}

void Pause::manual_unload_process(Response response) {
    const bool can_continue = !FSensors_instance().has_filament_surely(LogicalFilamentSensor::extruder);
    setPhase(can_continue ? PhasesLoadUnload::ManualUnload_continuable : PhasesLoadUnload::ManualUnload_uncontinuable);
    handle_help(response);

    if (response == Response::Continue
        && can_continue) { // Allow to continue when nothing remains in filament sensor
        enable_e_steppers();
        set(LoadState::unload_finish_or_change);

    } else if (response == Response::Retry) { // Retry unloading
        enable_e_steppers();
        set(LoadState::ram_sequence);
    }
}

bool Pause::tool_change([[maybe_unused]] uint8_t target_extruder, [[maybe_unused]] LoadType load_type_,
    [[maybe_unused]] const pause::Settings &settings_) {
#if HAS_TOOLCHANGER()
    if (target_extruder != active_extruder) {
        settings = settings_;
        load_type = load_type_;

        // Remove XY park position before toolchange, it will park in next operation
        settings.park_pos.x = std::numeric_limits<float>::quiet_NaN();
        settings.park_pos.y = std::numeric_limits<float>::quiet_NaN();

        // Park Z and show toolchange screen
        FSM_HolderLoadUnload holder(*this);
        setPhase(PhasesLoadUnload::ChangingTool);

        // Change tool, don't lift or return Z as it was done by parking
        return prusa_toolchanger.tool_change(target_extruder, tool_return_t::no_return, current_position, tool_change_lift_t::no_lift, false);
    }
#endif /*HAS_TOOLCHANGER()*/

    return true;
}

bool Pause::perform(LoadType load_type_, const pause::Settings &settings_) {
    load_type = load_type_;
    settings = settings_;
    return invoke_loop();
}

bool Pause::invoke_loop() {
#if ENABLED(PID_EXTRUSION_SCALING)
    bool extrusionScalingEnabled = thermalManager.getExtrusionScalingEnabled();
    thermalManager.setExtrusionScalingEnabled(false);
#endif // ENABLED(PID_EXTRUSION_SCALING)

    FSM_HolderLoadUnload holder(*this);

    // Prevent the "waiting for temperature restore" from triggering - the Pause manages temperature safety for extrusion internally
    buddy::SafetyTimerNonBlockingGuard non_blocking_guard;

    set(LoadState::start);

    while (!finished()) {
        auto response { getResponse() };
        if (planner.draining() || check_user_stop(response)) {
            set(LoadState::stop);
        }
        (this->*(state_handlers[state]))(response);
        idle(true); // idle loop to prevent wdt and manage heaters etc
    };

#if ENABLED(PID_EXTRUSION_SCALING)
    thermalManager.setExtrusionScalingEnabled(extrusionScalingEnabled);
#endif // ENABLED(PID_EXTRUSION_SCALING)

    return finished_ok();
}

/*****************************************************************************/
// park moves
uint32_t Pause::parkMoveZPercent(float z_move_len, float xy_move_len) const {
    const float Z_time_ratio = std::abs(z_move_len / float(NOZZLE_PARK_Z_FEEDRATE));
    const float XY_time_ratio = std::abs(xy_move_len / float(NOZZLE_PARK_XY_FEEDRATE));

    if (!isfinite(Z_time_ratio)) {
        return 100;
    }
    if (!isfinite(XY_time_ratio)) {
        return 0;
    }
    if ((Z_time_ratio + XY_time_ratio) == 0) {
        return 50; // due abs should not happen except both == 0
    }

    return 100.f * (Z_time_ratio / (Z_time_ratio + XY_time_ratio));
}

uint32_t Pause::parkMoveXYPercent(float z_move_len, float xy_move_len) const {
    return 100 - parkMoveZPercent(z_move_len, xy_move_len);
}

bool Pause::parkMoveXGreaterThanY(const xyz_pos_t &pos0, const xyz_pos_t &pos1) const {
    xy_bool_t pos_nan;
    LOOP_XY(axis) {
        pos_nan.pos[axis] = isnan(pos0.pos[axis]) || isnan(pos1.pos[axis]);
    }

    if (pos_nan.y) {
        return true;
    }
    if (pos_nan.x) {
        return false;
    }

    return std::abs(pos0.x - pos1.x) > std::abs(pos0.y - pos1.y);
}

[[nodiscard]] Pause::StopConditions Pause::wait_for_motion_finish_stoppable(StopConditions check_for /* = MotionStoppableConditions::UserStopped */) {
    while (planner.processing()) {
        if (check4(check_for, StopConditions::UserStopped) && check_user_stop(getResponse())) {
            return StopConditions::UserStopped;
        }
        if (check4(check_for, StopConditions::SideFilamentSensorRunout) && FSensors_instance().no_filament_surely(LogicalFilamentSensor::side)) {
            log_info(MarlinServer, "Pause::sideFS runout");
            // Discard planned and executed moves at once - a bit brute-force solution, but there are currently no other planned moves than the E-move
            planner.quick_stop_and_resume();

            return StopConditions::SideFilamentSensorRunout;
        }
        idle(true);
    }
    return StopConditions::Accomplished;
}

void Pause::park_nozzle_and_notify() {
    setPhase(is_unstoppable() ? PhasesLoadUnload::Parking_unstoppable : PhasesLoadUnload::Parking_stoppable);
    // Initial retract before move to filament change position
    if (settings.retract && thermalManager.hotEnoughToExtrude(active_extruder)) {
        do_pause_e_move(settings.retract, PAUSE_PARK_RETRACT_FEEDRATE);
    }

    const float target_Z = settings.park_pos.z;
    const float Z_len = current_position.z - target_Z; // sign does not matter
    const float Z_feedrate = settings.park_z_feedrate; // customizable Z feedrate

    float XY_len = 0;
    float begin_pos = 0;
    float end_pos = 0;
    const bool x_greater_than_y = parkMoveXGreaterThanY(current_position, settings.park_pos);
    if (x_greater_than_y) {
        if (!isnan(settings.park_pos.x)) {
            begin_pos = axes_need_homing(_BV(X_AXIS)) ? float(X_HOME_POS) : current_position.x;
            end_pos = settings.park_pos.x;
            XY_len = begin_pos - end_pos; // sign does not matter
        }
    } else {
        if (!isnan(settings.park_pos.y)) {
            begin_pos = axes_need_homing(_BV(Y_AXIS)) ? float(Y_HOME_POS) : current_position.y;
            end_pos = settings.park_pos.y;
            XY_len = begin_pos - end_pos; // sign does not matter
        }
    }

    // move by z_lift, scope for PauseFsmExplicitProgressNotifier
    if (isfinite(target_Z)) {
        if (axes_need_homing(_BV(Z_AXIS))) {
            unhomed_z_lift(target_Z);
        } else {
            PauseFsmExplicitProgressNotifier N(*this, current_position.z, target_Z, 0, parkMoveZPercent(Z_len, XY_len), marlin_vars().native_pos[MARLIN_VAR_INDEX_Z]);
            log_info(MarlinServer, "Parking");
            plan_park_move_to(current_position.x, current_position.y, target_Z, NOZZLE_PARK_XY_FEEDRATE, Z_feedrate, Segmented::yes);
            log_info(MarlinServer, "Park done");
            if (wait_for_motion_finish_stoppable() == StopConditions::UserStopped) {
                return;
            }
        }
    }

    // move to (x_pos, y_pos)
    if (XY_len != 0) {
#if CORE_IS_XY
        if (axes_need_homing(_BV(X_AXIS) | _BV(Y_AXIS))) {
            GcodeSuite::G28_no_parser(true, true, false,
                {
                    .only_if_needed = true,
                    .z_raise = 0,
                    .precise = false, // We don't need precise position for this procedure
                });

            // We have moved both axes, go to park position if not requested otherwise
            static constexpr xyz_pos_t park = XYZ_NOZZLE_PARK_POINT_M600;
            LOOP_XY(axis) {
                if (isnan(settings.park_pos.pos[axis])) {
                    settings.park_pos.pos[axis] = park[axis];
                }
            }
        } else {
            LOOP_XY(axis) {
                if (isnan(settings.park_pos.pos[axis])) {
                    settings.park_pos.pos[axis] = current_position.pos[axis];
                }
            }
        }
#else /*CORE_IS_XY*/
        // home the X or Y axis if it is not homed and we want to move it
        // homing is after Z move to be clear of all obstacles
        // Should not affect other operations than Load/Unload/Change filament run from home screen without homing. We are homed during print
        LOOP_XY(axis) {
            // TODO: make homeaxis non-blocking to allow quick_stop
            if (!isnan(settings.park_pos.pos[axis])) {
                GcodeSuite::G28_no_parser(axis == X_AXIS, axis == Y_AXIS, false,
                    {
                        .only_if_needed = true,
                        .z_raise = 0,
                        .precise = false, // We don't need precise position for this procedure
                    });
            }
            if (check_user_stop(getResponse())) {
                return;
            }
            if (isnan(settings.park_pos.pos[axis])) {
                settings.park_pos.pos[axis] = current_position.pos[axis];
            }
        }
#endif /*CORE_IS_XY*/

        PauseFsmExplicitProgressNotifier N(*this, begin_pos, end_pos, parkMoveZPercent(Z_len, XY_len), 100, marlin_vars().native_pos[x_greater_than_y ? MARLIN_VAR_INDEX_X : MARLIN_VAR_INDEX_Y]); // from Z% to 100%
        plan_park_move_to_xyz(settings.park_pos, NOZZLE_PARK_XY_FEEDRATE, Z_feedrate, Segmented::yes);
        if (wait_for_motion_finish_stoppable() == StopConditions::UserStopped) {
            return;
        }
    }

    report_current_position();
}

void Pause::unpark_nozzle_and_notify() {
    if (settings.resume_pos.x == NAN || settings.resume_pos.y == NAN || settings.resume_pos.z == NAN) {
        return;
    }

    setPhase(PhasesLoadUnload::Unparking);
    // Move XY to starting position, then Z
    const bool x_greater_than_y = parkMoveXGreaterThanY(current_position, settings.resume_pos);
    const float &begin_pos = x_greater_than_y ? current_position.x : current_position.y;
    const float &end_pos = x_greater_than_y ? settings.resume_pos.x : settings.resume_pos.y;

    const float Z_len = current_position.z - settings.resume_pos.z; // sign does not matter, does not check Z max val (unlike park_nozzle_and_notify)
    const float XY_len = begin_pos - end_pos; // sign does not matter

    // home the axis if it is not homed
    // we can move only one axis during parking and not home the other one and then unpark and move the not homed one, so we need to home it
    GcodeSuite::G28_no_parser(!isnan(settings.park_pos.pos[X_AXIS]), !isnan(settings.park_pos.pos[Y_AXIS]), false,
        {
            .only_if_needed = true,
            .z_raise = 0,
            .precise = false, // We don't need precise position for this procedure
        });

    {
        PauseFsmExplicitProgressNotifier N(*this, begin_pos, end_pos, 0, parkMoveXYPercent(Z_len, XY_len), marlin_vars().native_pos[x_greater_than_y ? MARLIN_VAR_INDEX_X : MARLIN_VAR_INDEX_Y]);
        do_blocking_move_to_xy(settings.resume_pos, NOZZLE_UNPARK_XY_FEEDRATE);
    }

    // Move Z_AXIS to saved position, scope for PauseFsmNotifier
    {
        auto resume_pos_adj = settings.resume_pos;

        // Gotta apply leveling, otherwise the move would move the axes to non-leveled coordinates
        // (because do_blocking_move_to->plan_park_move_to->buffer_line doesn't apply the leveling :/)
        // If PLANNER_LEVELING is true, the leveling is applied inside buffer_line
#if HAS_LEVELING && !PLANNER_LEVELING
        planner.apply_leveling(resume_pos_adj);
#endif

        PauseFsmExplicitProgressNotifier N(*this, current_position.z, resume_pos_adj.z, parkMoveXYPercent(Z_len, XY_len), 100, marlin_vars().native_pos[MARLIN_VAR_INDEX_Z]); // from XY% to 100%
        // FIXME: use a beter movement api when available
        do_blocking_move_to_z(resume_pos_adj.z, feedRate_t(NOZZLE_PARK_Z_FEEDRATE), Segmented::yes);
        // But since the plan_park_move_to overrides the current position values (which are by default in
        // native (without MBL) coordinates and we apply MBL to them) we need to reset the z height to
        // make all the future moves correct.
        current_position.z = settings.resume_pos.z;
    }

    // Unretract
    if (settings.retract) {
        plan_e_move(-settings.retract, PAUSE_PARK_RETRACT_FEEDRATE);
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
 * - Move the nozzle back to resume_position
 * - Sync the planner E to resume_position.e
 * - Send host action for resume, if configured
 * - Resume the current SD print job, if any
 */
void Pause::filament_change(const pause::Settings &settings_, bool is_filament_stuck) {
    settings = settings_;

    load_type = is_filament_stuck ? LoadType::filament_stuck : LoadType::filament_change;

    if (did_pause_print) {
        return; // already paused
    }

    // Restore target temperatures, otherwise targetTooColdToExtrude would return true
    buddy::safety_timer().reset_restore_nonblocking();

    if (!DEBUGGING(DRYRUN) && settings.unload_length && thermalManager.targetTooColdToExtrude(settings.target_extruder)) {
        SERIAL_ECHO_MSG(MSG_ERR_HOTEND_TOO_COLD);
        return; // unable to reach safe temperature
    }

    // Lock filament sensor, so it does not enqueue new M600, beacuse of filament run out
    FS_EventAutolock runout_disable;

    // Indicate that the printer is paused
    ++did_pause_print;

    print_job_timer.pause();

    // Wait for buffered blocks to complete
    planner.synchronize();

#if ENABLED(ADVANCED_PAUSE_FANS_PAUSE) && FAN_COUNT > 0
    thermalManager.set_fans_paused(true);
#endif

    invoke_loop();

#if ADVANCED_PAUSE_RESUME_PRIME != 0
    do_pause_e_move(ADVANCED_PAUSE_RESUME_PRIME, feedRate_t(ADVANCED_PAUSE_PURGE_FEEDRATE));
#endif

    // Now all extrusion positions are resumed and ready to be confirmed
    // Set extruder to saved position
    planner.set_e_position_mm((destination.e = current_position.e = settings.resume_pos.e));

    --did_pause_print;

#if ENABLED(ADVANCED_PAUSE_FANS_PAUSE) && FAN_COUNT > 0
    thermalManager.set_fans_paused(false);
#endif

    // Resume the print job timer if it was running
    if (print_job_timer.isPaused()) {
        print_job_timer.start();
    }

#if ENABLED(EXTENSIBLE_UI)
    ui.reset_status();
#endif
}

bool Pause::ram_filament() {
    if (!ensureSafeTemperatureNotifyProgress()) {
        return false;
    }

    setPhase(is_unstoppable() ? PhasesLoadUnload::Ramming_unstoppable : PhasesLoadUnload::Ramming_stoppable);

    const RammingSequence *ramming_sequence = nullptr;

    switch (load_type) {
    case LoadType::filament_change:
    case LoadType::filament_stuck:
        ramming_sequence = &runoutRammingSequence;
        break;

    default:
        ramming_sequence = &unloadRammingSequence;
        break;
    }

    PauseFsmDurationNotifier notifier(*this, ramming_sequence->duration_estimate_ms());
    ram_retracted_distance = ramming_sequence->retracted_distance();
    ramming_sequence->execute([this] {
        return !check_user_stop(getResponse());
    });
    return true;
}

void Pause::unload_filament() {
    const float saved_acceleration = planner.user_settings.retract_acceleration;
    {
        auto s = planner.user_settings;
        s.retract_acceleration = FILAMENT_CHANGE_UNLOAD_ACCEL;
        planner.apply_settings(s);
    }

    // The ramming that happened before the unload already resulted in some amount of retraction -> subtract that
    const float remaining_unload_length = std::max<float>(std::abs(settings.unload_length) - ram_retracted_distance, 0);

    // At this point, we are already rammed (so the filament is out of the nozzle), so we do not need to enforce nozzle temp
    std::ignore = do_e_move_notify_progress_coldextrude(-remaining_unload_length, (FILAMENT_CHANGE_UNLOAD_FEEDRATE), StopConditions::UserStopped);

    {
        auto s = planner.user_settings;
        s.retract_acceleration = saved_acceleration;
        planner.apply_settings(s);
    }
}

bool Pause::check_user_stop(Response response) {
    if (response != Response::Stop) {
        return false;
    }
    set(LoadState::stop);
    return true;
}

void Pause::handle_filament_removal(LoadState state_to_set) {
    // only if there is no filament present and we are sure (FS on and sees no filament)
    if (FSensors_instance().no_filament_surely(LogicalFilamentSensor::extruder)) {
        set(state_to_set);
        config_store().set_filament_type(settings.GetExtruder(), FilamentType::none);
        return;
    }
    return;
}

void Pause::handle_help(Response response) {
    if (response != Response::Help) {
        return;
    }

    WarningType warning = WarningType::FilamentSensorStuckHelp;
#if HAS_MMU2()
    if (MMU2::mmu2.Enabled()) {
        // MMU requires filament sensors to function, do not offer disabling them
        warning = WarningType::FilamentSensorStuckHelpMMU;
    }
#endif

    if (marlin_server::prompt_warning(warning) == Response::FS_disable) {
        FSensors_instance().set_enabled_global(false);
        while (FSensors_instance().is_enable_state_update_processing()) {
            // Wait for the filament sensor disable to propagate, some phases rely on it to be updated
            idle(true);
        }
        marlin_server::set_warning(WarningType::FilamentSensorsDisabled);
        config_store().show_fsensors_disabled_warning_after_print.set(true);
    }
}

void Pause::setup_progress_mapper() {
    using LoadState = PausePrivatePhase::LoadState;
    using WorkflowStep = ProgressMapperWorkflowStep<LoadState>;

    const ProgressMapperWorkflow<LoadState> *result = nullptr;

    switch (load_type) {
    case LoadType::load_to_gears: {
        constexpr static ProgressMapperWorkflowArray workflow { std::to_array<WorkflowStep>({
            { LoadState::load_to_gears, 1 },
        }) };
        result = &workflow;
        break;
    }

    case LoadType::load:
    case LoadType::autoload: {
        constexpr static ProgressMapperWorkflowArray workflow { std::to_array<WorkflowStep>({
            // In autoload, first M1701 is issued with LoadType::load_to_gears where LoadState::load_to_gears is the only step
            // Then, LoadType::autoload follows where LoadState::load_to_gears is not done. But that's fine, ProgressMapper will just skip it.
            // In LoadType::load, LoadState::load_to_gears is part of the FSM
            { LoadState::load_to_gears, 1 },
                { LoadState::load_wait_temp, 3 },
                { LoadState::long_load, 1 },
                { LoadState::purge, 1 },
#if HAS_AUTO_RETRACT()
                { LoadState::auto_retract, 1 },
#endif
        }) };
        result = &workflow;
        break;
    }

    case LoadType::load_purge: {
        constexpr static ProgressMapperWorkflowArray workflow { std::to_array<WorkflowStep>({
            { LoadState::load_wait_temp, 3 },
                { LoadState::purge, 1 },
#if HAS_AUTO_RETRACT()
                { LoadState::auto_retract, 1 },
#endif
        }) };
        result = &workflow;
        break;
    }

    case LoadType::unload:
    case LoadType::unload_confirm:
    case LoadType::filament_stuck: {
        constexpr static ProgressMapperWorkflowArray workflow { std::to_array<WorkflowStep>({
            { LoadState::unload_wait_temp, 3 },
            { LoadState::ram_sequence, 2 },
            { LoadState::unload, 1 },
        }) };
        result = &workflow;
        break;
    }

    case LoadType::unload_from_gears: {
        constexpr static ProgressMapperWorkflowArray workflow { std::to_array<WorkflowStep>({
            { LoadState::unload_from_gears, 1 },
        }) };
        result = &workflow;
        break;
    }

    case LoadType::filament_change: {
        constexpr static ProgressMapperWorkflowArray workflow { std::to_array<WorkflowStep>({
            { LoadState::unload_wait_temp, 3 },
                { LoadState::ram_sequence, 1 },
                { LoadState::long_load, 2 },
                { LoadState::purge, 1 },
#if HAS_AUTO_RETRACT()
                { LoadState::auto_retract, 1 },
#endif
        }) };
        result = &workflow;
        break;
    }
    };

    progress_mapper.setup(*result);
}

/*****************************************************************************/
// Pause::FSM_HolderLoadUnload

Pause::FSM_HolderLoadUnload::FSM_HolderLoadUnload(Pause &p)
    : FSM_Holder(PhasesLoadUnload::initial)
    , pause(p) {
    pause.set_mode(pause.get_load_unload_mode());
    if (pause.should_park()) {
        pause.park_nozzle_and_notify();
    }
    active = true;
    // Turn off print fan during purging to prevent messy purging
    original_print_fan_speed = thermalManager.get_fan_speed(0);
    thermalManager.set_fan_speed(0, 0);
}

Pause::FSM_HolderLoadUnload::~FSM_HolderLoadUnload() {
    thermalManager.set_fan_speed(0, original_print_fan_speed);
    active = false;

    const float min_layer_h = 0.05f;
    // do not unpark and wait for temp if not homed or z park len is 0
    if (!axes_need_homing() && pause.settings.resume_pos.z != NAN && std::abs(current_position.z - pause.settings.resume_pos.z) >= min_layer_h && (marlin_client::is_printing() || marlin_client::is_paused())) {
        if (!pause.ensureSafeTemperatureNotifyProgress()) {
            return;
        }
        pause.unpark_nozzle_and_notify();
    }
    pause.clr_mode();
}

bool Pause::FSM_HolderLoadUnload::active = false;
