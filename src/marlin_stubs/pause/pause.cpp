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
#include <utils/variant_utils.hpp>

#include <option/has_crash_detection.h>
#include <option/has_pause.h>
static_assert(HAS_PAUSE());

#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include "Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#endif

#include "Marlin/src/lcd/extensible_ui/ui_api.h"
#include "Marlin/src/core/language.h"
#include "Marlin/src/lcd/ultralcd.h"

#if HAS_NOZZLE_CLEANER()
    #include <nozzle_cleaner.hpp>
#endif

#include "Marlin/src/feature/pause.h"
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include "marlin_server.hpp"
#include "fs_event_autolock.hpp"
#include "filament.hpp"
#include "client_response.hpp"
#include "fsm_loadunload_type.hpp"
#include <raii/auto_restore.hpp>
#include "mapi/motion.hpp"
#include <cmath>
#include <logging/log.hpp>
#include <config_store/store_instance.hpp>
#include <raii/scope_guard.hpp>
#include <filament_to_load.hpp>
#include <common/marlin_client.hpp>
#include <common/mapi/parking.hpp>
#include <feature/ramming/ramming_sequence.hpp>
#include <feature/ramming/standard_ramming_sequence.hpp>
#include <utils/progress.hpp>
#include <bsod/bsod.h>
#include <sound.hpp>
#include <feature/safety_timer/safety_timer.hpp>
#include <mapi/cold_extrude.hpp>
#include <mapi/feedrates/standard_feedrates.hpp>
#include <feature/gcode_exception/gcode_exception.hpp>

#include <option/has_human_interactions.h>
#include <option/has_wastebin.h>
#include <option/has_side_fsensor.h>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

#include <option/has_indx.h>
#if HAS_INDX()
    #include <Marlin/src/module/tool_change.h>
#endif
#include <option/has_extruder_fsensor.h>

#include <option/has_auto_retract.h>
#if HAS_AUTO_RETRACT()
    #include <feature/auto_retract/auto_retract.hpp>
#endif

#include <option/has_anfc.h>
#if HAS_ANFC()
    #include <feature/openprinttag/tool_tag.hpp>
    #include <feature/openprinttag/filament_usage_tracker/filament_usage_tracker.hpp>
#endif

#if HAS_CRASH_DETECTION()
    #include <feature/prusa/crash_recovery.hpp>
#endif

#include <option/has_filament_tracker.h>
#if HAS_FILAMENT_TRACKER()
    #include <feature/filament_tracker/filament_tracker.hpp>
#endif

LOG_COMPONENT_REF(MarlinServer);

#ifndef NOZZLE_UNPARK_XY_FEEDRATE
    #define NOZZLE_UNPARK_XY_FEEDRATE NOZZLE_PARK_XY_FEEDRATE
#endif

// private:
// check unsupported features
// filament sensor is no longer part of marlin thus it must be disabled
// clang-format off
#if (!ENABLED(EXTENSIBLE_UI))
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
class PauseFsmNotifier : public Subscriber<> {

public:
    PauseFsmNotifier(Pause &pause, float min, float max, const MarlinVariable<float> &var)
        : Subscriber<>(marlin_server::idle_publisher, [this, &var] {
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
class PauseFsmDurationNotifier : public Subscriber<> {

public:
    PauseFsmDurationNotifier(Pause &pause, uint32_t duration_ms)
        : Subscriber<>(marlin_server::idle_publisher, [this, duration_ms] {
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
class PauseFsmExplicitProgressNotifier : public Subscriber<> {

public:
    PauseFsmExplicitProgressNotifier(Pause &pause, float min, float max, ProgressPercent progress_min, ProgressPercent progress_max, const MarlinVariable<float> &var)
        : Subscriber<>(marlin_server::idle_publisher, [this, &var] {
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

// cannot be class member (externed in marlin and used by tool_change)
void do_pause_e_move(const float &length, const feedRate_t &fr_mm_s) {
    mapi::extruder_move(length, fr_mm_s);
    planner.synchronize();
}

void unhomed_z_lift(float amount_mm) {
    if (amount_mm > current_position.z) {
        do_homing_move((AxisEnum)(Z_AXIS), amount_mm - current_position.z, HOMING_FEEDRATE_INVERTED_Z, false); // warning: the speed must probably be exactly this, otherwise endstops don't work
    }
}

/*****************************************************************************/
// PausePrivatePhase

PausePrivatePhase::PausePrivatePhase()
    : phase(PhasesLoadUnload::initial) {
}

void PausePrivatePhase::setPhase(PhasesLoadUnload ph) {
    phase = ph;
    if (load_unload_mode) {
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

#if HAS_SPOOL_JOIN() && HAS_TOOLCHANGER()
    case LoadType::unload_spool_join:
        // If the unload during spool join does not finish for some reason,
        // it's not a problem - the printer will just pick a next nozzle and yadida
        return false;
#endif
    }

    bsod_unreachable();
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
#if HAS_SPOOL_JOIN() && HAS_TOOLCHANGER()
    case LoadType::unload_spool_join:
#endif
        return LoadUnloadMode::Unload;

    case Pause::LoadType::filament_change:
        return LoadUnloadMode::Change;

    case Pause::LoadType::filament_stuck:
        return LoadUnloadMode::FilamentStuck;
    }

    bsod_unreachable();
}

bool Pause::should_park() {
    auto virtual_tool = stdext::get_optional<VirtualToolIndex>(VirtualToolIndex::currently_selected());
    bool is_already_parked = !virtual_tool.has_value();
    if (is_already_parked) {
        return false;
    }

    switch (load_type) {
    case LoadType::load_purge:
    case LoadType::filament_change:
    case LoadType::filament_stuck:
        return true;

    case LoadType::load_to_gears:
        return !FSensors_instance().has_filament_surely(LogicalFilamentSensor::extruder);

    case LoadType::autoload:
        // TODO: Change autoload trigger sensor on printers with side_fs
        // and adjust phases to handle properly loading to gears, mmu_rework and parking
        // autoload on printers with side_fs, should behave similary to iX autoload

    case LoadType::load:
        return option::has_human_interactions || !FSensors_instance().has_filament_surely(LogicalFilamentSensor::extruder);

    case LoadType::unload_from_gears:
        return false;

    case LoadType::unload:
    case LoadType::unload_confirm:
#if HAS_SPOOL_JOIN() && HAS_TOOLCHANGER()
    case LoadType::unload_spool_join:
#endif
        return needs_hot_nozzle(load_type, settings.physical_tool());
    }

    bsod_unreachable();
}

bool Pause::is_target_temperature_safe() {
    // Restore target temperatures, otherwise targetTooColdToExtrude would return true
    buddy::safety_timer().reset_restore_nonblocking();

    if (!needs_hot_nozzle(load_type, settings.physical_tool())) {
        return true; // Its safe to unload even if the temp is too low if we are retracted
    }

    if (!DEBUGGING(DRYRUN) && thermalManager.targetTooColdToExtrude(settings.physical_tool())) {
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
        Temperature::degTargetHotend(active_extruder) - heating_phase_min_hotend_diff, marlin_vars().hotend(settings.physical_tool()).temp_nozzle);

    // Wait until temperature is close
    while (!is_tempreature_reached()) {
        if (check_user_stop(getResponse()) || gcode_exceptions().is_unwinding()) {
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
#if HAS_AUTO_RETRACT()
    if (length > 0) {
        // Deretract before spinning up the progress notifier
        // It would be tracking the deretract moves, causing erratic progress jumps as the auto_retract resets the E position to the original one at the end
        auto_retract().maybe_deretract_to_nozzle();

        // Make sure marlin_vars().native_pos is up to date
        idle(false);
    }
#endif

    PauseFsmNotifier N(*this, current_position.e, current_position.e + length, marlin_vars().native_pos[MARLIN_VAR_INDEX_E]);

    mapi::extruder_move(length, fr_mm_s);

    return wait_for_motion_finish_stoppable(check_for);
}

[[nodiscard]] Pause::StopConditions Pause::do_e_move_notify_progress_coldextrude(const float &length, const feedRate_t &fr_mm_s, StopConditions check_for) {
    mapi::ColdExtrudeGuard cold_extrude_guard;
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
#if HAS_SPOOL_JOIN() && HAS_TOOLCHANGER()
    case LoadType::unload_spool_join:
#endif
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

#if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
    if (FSensors_instance().has_filament_surely(LogicalFilamentSensor::extruder) && FSensors_instance().no_filament_surely(LogicalFilamentSensor::side)) {
        // When filament is in extruder sensor but not in side sensor, it's not a good idea to push another one in

        // Filament should be already out of gears by now, we move it just to be sure it's removable manually
        std::ignore = do_e_move_notify_progress_coldextrude(
            -20.f, standard_feedrates::extruder(standard_feedrates::Extruder::filament_unload, filament::get_type_to_load()), StopConditions::Accomplished);
        sound::play(SoundType::single_beep);
        set(LoadState::loading_obstruction);
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
        set(LoadState::load_wait_temp);
        handle_filament_removal(LoadState::filament_push_ask);
        break;
    case LoadType::load_purge:
        set(LoadState::load_wait_temp);
        break;
    default:
        if (option::has_side_fsensor && !FSensors_instance().is_extruder_fs_independent()) {
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

#if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
void Pause::loading_obstruction_process(Response response) {
    setPhase(is_unstoppable() ? PhasesLoadUnload::LoadingObstruction_unstoppable : PhasesLoadUnload::LoadingObstruction_stoppable);
    handle_help(response);

    switch (response) {
    case Response::Help:
    case Response::Retry:
        // Retry falls back to load_start, where FS is checked again and retracts if it fails
        set(LoadState::load_start);
        break;

    case Response::Stop:
        set(LoadState::stop);
        break;

    default:
        break;
    }
}
#endif

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
        if (!FSensors_instance().is_extruder_fs_independent()) {
#if ENABLED(PREVENT_COLD_EXTRUSION)
            mapi::ColdExtrudeGuard cold_extrude_guard;
#endif
            mapi::extruder_schedule_turning(
                standard_feedrates::extruder(standard_feedrates::Extruder::filament_assisted, filament::get_type_to_load()));
        }

    } else {
        setPhase(is_unstoppable() ? PhasesLoadUnload::UserPush_unstoppable : PhasesLoadUnload::UserPush_stoppable);
        const bool has_filament = FSensors_instance().has_filament_surely(LogicalFilamentSensor::extruder);
        const bool is_mmu_rework_and_has_filament = !FSensors_instance().is_extruder_fs_independent() && has_filament;
        const bool side_fs_empty = FSensors_instance().no_filament_surely(LogicalFilamentSensor::side);
        const bool extruder_fs_not_working = FSensors_instance().sensor(LogicalFilamentSensor::extruder) && !FSensors_instance().is_working(LogicalFilamentSensor::extruder);

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
        mapi::home_if_needed_and_park(mapi::get_parking_position(mapi::ParkPosition::load).without_z_move());
        if (!FSensors_instance().is_extruder_fs_independent()) {
            set_timed(LoadState::assist_insertion);
        } else {
            set(LoadState::filament_push_ask);
        }
        return;
    }
}

void Pause::runout_during_load_process([[maybe_unused]] Response response) {
    setPhase(PhasesLoadUnload::Ejecting_unstoppable);
#if HAS_INDX() // We need to extrude a bit (at least 2mm) to lock the head locking mechanism (it is partialy unlocked after toolpick from the tool pick) before doing retracting moves.
    std::ignore = do_e_move_notify_progress_coldextrude(
        3.0f, standard_feedrates::extruder(standard_feedrates::Extruder::filament_unload, filament::get_type_to_load()), StopConditions::Accomplished);
#endif

    // unload immediately - we even cannot perform ramming as it would have consumed even more filament
    std::ignore = do_e_move_notify_progress_coldextrude(
        -std::abs(settings.unload_length), standard_feedrates::extruder(standard_feedrates::Extruder::filament_unload, filament::get_type_to_load()), StopConditions::Accomplished);

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

    // Load timeout before giving up
    if (ticks_diff(ticks_ms(), start_time_ms) > 120 * 1000) {
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
    mapi::ColdExtrudeGuard cold_extrude_guard;
#endif
    // Enqueue an E move, but only if there are no more than 4 moves scheduled.
    // This ensures that there is always 0.4mm of movement enqueued in advance,
    // Guaranteeing a maximum movement difference of 0.1mm
    mapi::extruder_schedule_turning(
        standard_feedrates::extruder(standard_feedrates::Extruder::filament_slow_load, filament::get_type_to_load()), 0.1f);
}

void Pause::load_to_gears_process([[maybe_unused]] Response response) { // slow load
    setPhase(is_unstoppable() ? PhasesLoadUnload::LoadingToGears_unstoppable : PhasesLoadUnload::LoadingToGears_stoppable);

    const auto result = do_e_move_notify_progress_coldextrude(
        settings.slow_load_length, standard_feedrates::extruder(standard_feedrates::Extruder::filament_slow_load, filament::get_type_to_load()), StopConditions::All);

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
        mapi::home_if_needed_and_park(mapi::get_parking_position(mapi::ParkPosition::purge).without_z_move());
    }
    set(LoadState::load_wait_temp);
}

void Pause::load_wait_temp_process([[maybe_unused]] Response response) {
    if (ensureSafeTemperatureNotifyProgress()) {
        // blocking -> checks for user stop

        const auto filament_to_load = filament::get_type_to_load();
        if (filament_to_load != NoFilamentType {}) {
            // We preheated to the higher temeperature of the filament currently
            // being loaded and the previously loaded filament. Drop the target to
            // the temperature of the filament currently being loaded just before
            // loading it into the nozzle (this will drop the nozzle temperature by
            // itself, overheating the new filament a bit less).
            thermalManager.setTargetHotend(
                filament_to_load.parameters().nozzle_temperature,
                settings.physical_tool());

        } else {
            // If we don't know the new material (can happen in M600 during print),
            // keep the original temperature instead of turning the nozzle off :3
        }

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

#if HAS_INDX()
    set(LoadState::unload_purge);
#else
    set(LoadState::ram_sequence);
#endif
}

void Pause::long_load_process([[maybe_unused]] Response response) {
    setPhase(is_unstoppable() ? PhasesLoadUnload::Loading_unstoppable : PhasesLoadUnload::Loading_stoppable);

    const float saved_acceleration = planner.user_settings.retract_acceleration;
    {
        auto s = planner.user_settings;
        s.retract_acceleration = FILAMENT_CHANGE_FAST_LOAD_ACCEL;
        planner.apply_settings(s);
    }

    auto move_e_progress = do_e_move_notify_progress_hotextrude(
        settings.fast_load_length, standard_feedrates::extruder(standard_feedrates::Extruder::filament_fast_load, filament::get_type_to_load()), StopConditions::All);

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

void Pause::purge_process([[maybe_unused]] Response response) {
    // Extrude filament to get into hotend
    setPhase(is_unstoppable() ? PhasesLoadUnload::Purging_unstoppable : PhasesLoadUnload::Purging_stoppable);

    planner.synchronize(); // Finish any pending moves before starting the purge

    const auto old_filament = config_store().get_filament_type(settings.virtual_tool());
    config_store().set_filament_type(settings.virtual_tool(), filament::get_type_to_load());

#if HAS_NOZZLE_CLEANER()
    const bool purge_ok = nozzle_cleaner_purge_sequence();
#else
    const bool purge_ok = standard_purge_sequence();
#endif
    if (!purge_ok) {
        config_store().set_filament_type(settings.virtual_tool(), old_filament);
        return;
    }

    if constexpr (option::has_human_interactions) {
        set(LoadState::color_correct_ask);
    } else {
        set(LoadState::load_finalize);
    }

    handle_filament_removal(LoadState::filament_push_ask);
}

bool Pause::standard_purge_sequence() {
    const auto purge_result = do_e_move_notify_progress_hotextrude(
        settings.purge_length(), standard_feedrates::extruder(standard_feedrates::Extruder::advanced_pause_purge, filament::get_type_to_load()), StopConditions::All);

    if (purge_result == StopConditions::SideFilamentSensorRunout) {
        set(LoadState::runout_during_load);
        return false;
    }
    // If the user stopped the purge, we need to stop the extruder move
    if (purge_result == StopConditions::UserStopped) {
        planner.quick_stop_and_resume();
    }
    // Skip retraction if Failed
    if (purge_result != StopConditions::Failed) {
        std::ignore = do_e_move_notify_progress_hotextrude(
            -STANDARD_RETRACT_LENGTH, standard_feedrates::extruder(standard_feedrates::Extruder::retract, filament::get_type_to_load()), StopConditions::UserStopped);
    }

    return true;
}

#if HAS_NOZZLE_CLEANER()
bool Pause::nozzle_cleaner_purge_sequence() {
    [[maybe_unused]] static constexpr float purge_length = 5.f;
    static constexpr uint8_t retry_cnt = 3; // Number of maximum retries for the whole nozzle cleaning purge loop
    static constexpr uint32_t purge_nozzle_clean_duration_estimate_ms = 8'000;
    PauseFsmDurationNotifier progress_notifier(*this, purge_nozzle_clean_duration_estimate_ms);

    ScopeGuard resetLoader = [&] {
        nozzle_cleaner::reset();
    };

    #if HAS_INDX()
    // Purge gcode sequences use relative E moves — save and restore the original mode
    const uint8_t saved_axis_relative = GcodeSuite::axis_relative;
    GcodeSuite::set_e_relative();
    ScopeGuard restore_e_mode = [saved_axis_relative] {
        GcodeSuite::axis_relative = saved_axis_relative;
    };
    #endif

    float purged = 0.f;
    while (purged < settings.purge_length()) {
        mapi::park(mapi::get_parking_position(mapi::ParkPosition::purge).without_z_move());
        planner.synchronize(); // Wait for the park to finish before continuing
    #if !HAS_INDX() // We do the purgue move in the gcode of the loader on INDX, so we don't want to do it here
        const auto purge_result = do_e_move_notify_progress_hotextrude(
            purge_length, standard_feedrates::extruder(standard_feedrates::Extruder::advanced_pause_purge, filament::get_type_to_load()), StopConditions::All);
        purged += purge_length;
        switch (purge_result) {
        case StopConditions::SideFilamentSensorRunout:
            set(LoadState::runout_during_load);
            return false;
        case StopConditions::UserStopped:
            planner.quick_stop_and_resume();
            set(LoadState::load_finalize);
            return false;
        default:
            break;
        }
    #endif
        planner.synchronize(); // Wait for the purge to finish before continuing
        if (!nozzle_cleaner::load_and_execute(nozzle_cleaner::Sequence::purge_clean)) {
            if (planner.draining()) {
                return false; // we exited the load_and_execute cause of .draining, we need to exit asap
            }

            if (++failed_purge_attempts >= retry_cnt) {
                // If we failed to purge the nozzle x times, we need to stop the process
                SERIAL_ECHO_MSG("Purging with nozzle cleaning failed, stopping the process");
                set(LoadState::stop);
                failed_purge_attempts = 0; // Reset the failed attempts counter
                return false;
            }
            return false; // This exits the loop and method and does not change the state so the whole loop will begin again
        };

    #if HAS_INDX() // INDX_TODO: clean up this :)
        break; // On INDX we do the purge in the gcode of the loader, so we only want to loop on the cleaner execution, but not do multiple purges
    #endif
    }

    return true;
}
#endif // HAS_NOZZLE_CLEANER()

void Pause::color_correct_ask_process(Response response) {
    setPhase(load_type == LoadType::load_purge ? PhasesLoadUnload::IsColorPurge : PhasesLoadUnload::IsColor);
    switch (response) {

    case Response::Purge_more:
#if HAS_NOZZLE_CLEANER()
        // On printers with nozzle cleaner, go through the full purge flow:
        // park to nozzle cleaner position → ensure temp → nozzle clean sequence
        load_type = LoadType::load_purge;
        set(LoadState::move_to_purge);
#else
        set(LoadState::purge);
#endif
        break;

    case Response::Retry:
        set(LoadState::eject);
        break;

    case Response::Yes:
        set(LoadState::load_finalize);
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
            set(LoadState::load_finalize);
            return;
        }
        config_store().set_filament_type(VirtualToolIndex::from_raw(settings.mmu_filament_to_load), filament::get_type_to_load());

        set(LoadState::color_correct_ask);
    } else if (load_type == LoadType::filament_change) {
        if (settings.mmu_filament_to_load == MMU2::FILAMENT_UNKNOWN) {
            set(LoadState::load_finalize);
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
        set(LoadState::load_finalize);
        return;
    }

    MMU2::mmu2.load_filament(settings.mmu_filament_to_load);
    MMU2::mmu2.load_filament_to_nozzle(settings.mmu_filament_to_load);

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

void Pause::load_finalize_process(Response) {
#if HAS_AUTO_RETRACT()
    // Only retract from nozzle outside printing
    if (!marlin_server::is_printing()) {
        // Needed for progress mapper
        // The process() of the state should never get called though, because end of this function switches to _finished
        set(LoadState::auto_retract);

        if (!ensureSafeTemperatureNotifyProgress()) {
            return;
        }

        setPhase(PhasesLoadUnload::AutoRetracting);
        const auto vt = stdext::get_optional<VirtualToolIndex>(VirtualToolIndex::currently_selected());
        PauseFsmDurationNotifier progress_notifier(*this, vt ? standard_ramming_sequence(StandardRammingSequence::auto_retract, *vt).duration_estimate_ms() : 0);
        auto_retract().maybe_retract_from_nozzle();

        // Set the state back, just in case
        set(LoadState::load_finalize);
    }
#else
    if constexpr (false) {
    }
#endif
    // Otherwise prime the nozzle
    else if (load_type == LoadType::filament_change || load_type == LoadType::filament_stuck) {
        if (!ensureSafeTemperatureNotifyProgress()) {
            return;
        }

        // Feed a little bit of filament to stabilize pressure in nozzle

        const auto filament = filament::get_type_to_load();

        // Last poop after user clicked color - yes
        plan_e_move(PARK_PAUSE_PRIME_LENGTH, standard_feedrates::extruder(standard_feedrates::Extruder::pause_prime, filament));

        // Retract again, it will be unretracted at the end of unpark
        if (settings.retract) {
            plan_e_move(settings.retract, standard_feedrates::extruder(standard_feedrates::Extruder::retract, filament));
        }

        planner.synchronize();
        delay(500);
    }

#if HAS_NOZZLE_CLEANER()
    {
        if (!ensureSafeTemperatureNotifyProgress()) {
            return;
        }

        // We cant use the regual load_and_execute cause we need to handle user_stop also
        nozzle_cleaner::load_sequence(nozzle_cleaner::Sequence::clean);
        setPhase(PhasesLoadUnload::LoadNozzleCleaning);
        while (!nozzle_cleaner::execute()) {
            if (planner.draining() || check_user_stop(getResponse())) {
                set(LoadState::stop);
                break;
            }
            idle(true);
        }

        if (!marlin_server::is_printing()) {
            // If not printing, park on the nozzle cleaner planchette
            mapi::park(mapi::get_parking_position(mapi::ParkPosition::park).without_z_move());
        }
    }
#endif

    set(LoadState::_finished);
}

void Pause::stop_process([[maybe_unused]] Response response) {
    if (!planner.busy()) {
        // The printer is not moving, we don't need to do anything drastic and lose homing.
        set(LoadState::_stopped);
        return;
    }

    planner.quick_stop_and_resume();
    xyze_pos_t real_current_position;
    planner.get_axis_position_mm(real_current_position);
    real_current_position[E_AXIS] = 0;
#if HAS_LEVELING
    planner.unapply_leveling(real_current_position);
#endif

    // Lose homing only if we interrupted XYZ movement. quick_stop on extruder movement is fine
    if (current_position.xyz() != real_current_position.xyz()) {
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
        // filament_stuck needs special handling in MMU-mode - it must be reported on the screen just like in normal mode
        // and the user must resolve it by hand. Currently, there is no way the MMU can safely help in resolving the error (too many potential edge-cases)
        if (load_type == LoadType::filament_stuck) {
    #if HAS_LOADCELL()
            set(LoadState::filament_stuck_ask);
    #endif
        } else {
            set(LoadState::mmu_unload_start);
        }
        return;
    }
#endif

#if HAS_ANFC()
    const auto tool = settings.virtual_tool();
    auto &fut = buddy::openprinttag::filament_usage_tracker();

    fut.flush({
        .tools = tool,
        // Don't warn - the PhasesLoadUnload::OPT_UncommitedUsage conveys the same information
        .warn_on_failure = false,
    });

    // Warn the user if there is some uncommited consumption and wait till it is written
    // If the uncommited usage resets to zero, automatically continues
    while (fut.is_tracking(tool) && fut.uncommited_consumption_mm(tool) > 5) {
        setPhase(PhasesLoadUnload::OPT_UncommitedUsage);

        if (marlin_server::get_response_from_phase(PhasesLoadUnload::OPT_UncommitedUsage) != Response::_none) {
            break;
        }

        if (gcode_exceptions().is_unwinding()) {
            return;
        }

        idle(true);
    }
#endif

    switch (load_type) {

    case LoadType::filament_stuck:
#if HAS_LOADCELL() && HAS_EXTRUDER_FSENSOR()
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

#if HAS_LOADCELL() && HAS_EXTRUDER_FSENSOR()
void Pause::filament_stuck_ask_process(Response response) {
    setPhase(PhasesLoadUnload::FilamentStuck);

    switch (response) {

    case Response::_none:
        break;

    case Response::Unload:
        set(LoadState::unload_wait_temp);
        break;

    case Response::Ignore:
        set(LoadState::_finished);
        break;

    case Response::Disable:
        config_store().stuck_filament_detection.set(false);
        set(LoadState::_finished);
        break;

    default:
        bsod_unreachable();
    }
}
#endif

#if HAS_INDX()
void Pause::unload_purge_process([[maybe_unused]] Response response) {
    setPhase(PhasesLoadUnload::Purging_unstoppable);

    static constexpr float unload_purge_length = 2.0f; // mm
    std::ignore = do_e_move_notify_progress_hotextrude(
        unload_purge_length, standard_feedrates::extruder(standard_feedrates::Extruder::advanced_pause_purge, FilamentType::for_tool_heuristic(settings.virtual_tool())), StopConditions::UserStopped);

    set(LoadState::ram_sequence);
}
#endif

void Pause::ram_sequence_process([[maybe_unused]] Response response) {
    auto physical_tool = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::currently_selected());
    if (!physical_tool.has_value()) {
        bsod("ramming with notool");
    }
#if HAS_AUTO_RETRACT()
    if (auto_retract().can_cold_unload(settings.physical_tool())) {
        // The filament is already retracted from the nozzle -> no ramming needed, we don't even need to heat up the nozzle
        ram_retracted_distance = auto_retract().retracted_distance(settings.physical_tool()).value(); // We are sure value is not std::nullopt because of can_cold_unload()
        set(LoadState::unload);
        return;
    }
#endif

    if (ram_filament()) {
        set(LoadState::unload);
    }
}

void Pause::unload_process([[maybe_unused]] Response response) {
    auto physical_tool = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::currently_selected());
    if (!physical_tool.has_value()) {
        bsod("unloading notool");
    }

    setPhase(is_unstoppable() ? PhasesLoadUnload::Unloading_unstoppable : PhasesLoadUnload::Unloading_stoppable);
#if HAS_NOZZLE_CLEANER()
    bool needs_cleaning = true; // Assume we need to clean the nozzle
    #if HAS_AUTO_RETRACT()
    needs_cleaning = !auto_retract().can_cold_unload(settings.physical_tool()); // If we are retracted, we don't need to clean the nozzle
    #endif
#endif
    unload_filament();

    config_store().set_filament_type(settings.virtual_tool(), FilamentType::none);

    switch (load_type) {
    case LoadType::unload:
#if HAS_SPOOL_JOIN() && HAS_TOOLCHANGER()
    case LoadType::unload_spool_join:
#endif
        if constexpr (option::has_human_interactions) {
            set(LoadState::unload_finish_or_change);
            break;
        }

    case LoadType::unload_confirm:
    case LoadType::filament_change:
    case LoadType::filament_stuck:
#if HAS_NOZZLE_CLEANER()
        if (needs_cleaning) {
            set(LoadState::unload_nozzle_clean);
            return;
        }
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
        // skip fsensor check for E-stall on MMU, FINDA remains pressed, because it makes no sense pulling filament out of the MMU as well.
        if (load_type == LoadType::filament_stuck && FSensors_instance().HasMMU()) {
            set(LoadState::filament_push_ask);
        } else {
            // On printers without extruder FSensor (like INDX) we want the user to confirm it without enforcing the filament to be removed from the sensor.
            // The motivation is that some users would remove the old and push the new one in first before confirming the unload, and if we enforce the
            // filament to be removed, the printer would think that the filament was not removed at all and would not proceed with loading the new filament.
            // The user would then need to remove the new filament again and push it in again after confirming the unload, which is a bad user experience.
            if (FSensors_instance().is_working(LogicalFilamentSensor::extruder)) {
                set(LoadState::filament_not_in_fs);
            } else {
                set(LoadState::unload_finish_or_change);
            }
        }
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
    std::ignore = do_e_move_notify_progress_coldextrude(
        -settings.slow_load_length * (float)1.5, standard_feedrates::extruder(standard_feedrates::Extruder::filament_fast_load, FilamentType::for_tool_heuristic(settings.virtual_tool())), StopConditions::UserStopped);
    set(LoadState::unload_finish_or_change);
}

#if HAS_NOZZLE_CLEANER()
void Pause::unload_nozzle_clean_process([[maybe_unused]] Response response) {
    setPhase(PhasesLoadUnload::UnloadNozzleCleaning);

    if (nozzle_cleaner::load_and_execute(nozzle_cleaner::Sequence::clean)) {
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
    if (!FSensors_instance().has_filament_surely(LogicalFilamentSensor::closest_to_nozzle_independent)) {
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
        }
    }
}

void Pause::manual_unload_process([[maybe_unused]] Response entry_response) {
#if HAS_INDX()
    // INDX carries the extruder in the toolhead - dock it (and back off from the dock) before the
    // user can remove the filament by hand. Remember where we started so we can return there after
    // re-picking the tool (instead of assuming the head started at a fixed position).
    const xyz_pos_t resume_pos = current_position.xyz();
    setPhase(PhasesLoadUnload::ChangingTool);
    // ::tool_change is the free function; the member Pause::tool_change would shadow it here
    ::tool_change(NoTool {}, tool_return_t::dock_backoff);

    // Re-pick the docked tool and return to where manual unload started
    const auto pick_tool_and_resume = [&] {
        setPhase(PhasesLoadUnload::ChangingTool);
        ::tool_change(settings.virtual_tool(), tool_return_t::no_return);
        mapi::park({ .x = resume_pos.x, .y = resume_pos.y, .z = resume_pos.z });
    };
#endif
    while (true) {
        const bool has_filament = FSensors_instance().has_filament_surely(LogicalFilamentSensor::extruder)
            || (!FSensors_instance().sensor(LogicalFilamentSensor::extruder) && FSensors_instance().has_filament_surely(LogicalFilamentSensor::side));
        const bool can_continue = !has_filament;
        const PhasesLoadUnload phase = can_continue ? PhasesLoadUnload::ManualUnload_continuable : PhasesLoadUnload::ManualUnload_uncontinuable;
        setPhase(phase);

        const Response response = getResponse();
        handle_help(response);

        if (planner.draining() || check_user_stop(response)) {
            set(LoadState::stop);
            return;
        }

        if (response == Response::Continue && can_continue) { // Allow to continue when nothing remains in filament sensor
            enable_e_steppers();
#if HAS_INDX()
            pick_tool_and_resume();
#endif
            set(LoadState::unload_finish_or_change);
            return;
        }

        if (response == Response::Retry) { // Retry unloading
            enable_e_steppers();
#if HAS_INDX()
            pick_tool_and_resume();
#endif
            set(LoadState::ram_sequence);
            return;
        }
        idle(true);
    }
}

bool Pause::tool_change([[maybe_unused]] VirtualToolIndex target_tool, [[maybe_unused]] LoadType load_type_,
    [[maybe_unused]] const pause::Settings &settings_) {
#if HAS_TOOLCHANGER()
    if (!stdext::holds_value(PhysicalToolIndex::currently_selected(), target_tool.to_physical())) {
        settings = settings_;
        load_type = load_type_;

        // Remove XY park position before toolchange, it will park in next operation
        settings.park_point.x = mapi::ParkingPosition::unchanged;
        settings.park_point.y = mapi::ParkingPosition::unchanged;

        // Park Z and show toolchange screen
        FSM_HolderLoadUnload holder(*this);
        setPhase(PhasesLoadUnload::ChangingTool);

        // Change tool, don't lift or return Z as it was done by parking
        return prusa_toolchanger.tool_change(target_tool.to_physical(), tool_return_t::no_return, current_position.xyz(), tool_change_lift_t::no_lift, false);
    }
#endif

    return true;
}

bool Pause::needs_hot_nozzle(LoadType lt, [[maybe_unused]] PhysicalToolIndex tool) {
    switch (lt) {

    case LoadType::load:
    case LoadType::autoload:
    case LoadType::load_purge:
        return true;

    case LoadType::filament_change:
    case LoadType::filament_stuck:
        return true;

    case LoadType::unload:
    case LoadType::unload_confirm:
#if HAS_SPOOL_JOIN() && HAS_TOOLCHANGER()
    case LoadType::unload_spool_join:
#endif
#if HAS_AUTO_RETRACT()
        return !auto_retract().can_cold_unload(tool);
#else
        return true;
#endif

    case LoadType::load_to_gears:
    case LoadType::unload_from_gears:
        // The filament is just grabbed by the gears, is not in the nozzle
        return false;
    }

    bsod_unreachable();
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

    // Trust that the filament change knows what it's doing
    AutoRestore ar_ce(thermalManager.allow_cold_extrude, true);

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

    return static_cast<uint32_t>(100.f * (Z_time_ratio / (Z_time_ratio + XY_time_ratio)));
}

uint32_t Pause::parkMoveXYPercent(float z_move_len, float xy_move_len) const {
    return 100 - parkMoveZPercent(z_move_len, xy_move_len);
}

bool Pause::parkMoveXGreaterThanY(const xyz_pos_t &pos0, const xyz_pos_t &pos1) const {
    xy_bool_t pos_nan;
    LOOP_XY(axis) {
        pos_nan[axis] = isnan(pos0[axis]) || isnan(pos1[axis]);
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

    const mapi::ParkingPosition &park = settings.park_point;
    // Resolve Z against the live position; resolve_z already clamps to Z_MAX_POS.
    const float target_Z = park.resolve_z(current_position.z);

    // Initial retract before move to filament change position
    if (!thermalManager.tooColdToExtrude(active_extruder)) {
        mapi::retract_to(-settings.retract, standard_feedrates::extruder(standard_feedrates::Extruder::retract, FilamentType::for_current_tool_heuristic()));
    }

    // Z lift
    if (isfinite(target_Z)) {
        if (axes_need_homing(_BV(Z_AXIS))) {
            unhomed_z_lift(target_Z);
        } else {
            log_info(MarlinServer, "Parking Z");
            mapi::park({ .z = target_Z });
        }
    }

    // Home XY if needed before parking
    const bool has_xy_park = !std::holds_alternative<mapi::ParkingPosition::Unchanged>(park.x) || !std::holds_alternative<mapi::ParkingPosition::Unchanged>(park.y);
    if (has_xy_park) {
        mapi::ParkingPosition xy_target = park;
#if CORE_IS_XY
        if (axes_need_homing(_BV(X_AXIS) | _BV(Y_AXIS))) {
            GcodeSuite::G28_no_parser(true, true, false,
                {
                    .only_if_needed = true,
                    .z_raise = 0,
                    .precise = false, // We don't need precise position for this procedure
                });

            // We have moved both axes, go to park position if not requested otherwise
            const mapi::ParkingPosition default_park = mapi::get_parking_position(mapi::ParkPosition::filament_change);
            if (std::holds_alternative<mapi::ParkingPosition::Unchanged>(xy_target.x)) {
                xy_target.x = default_park.x;
            }
            if (std::holds_alternative<mapi::ParkingPosition::Unchanged>(xy_target.y)) {
                xy_target.y = default_park.y;
            }
        }
#else /*CORE_IS_XY*/
        // Home each park axis individually; homing is after Z move to be clear of all obstacles.
        const xyz_bool_t is_park_axis = park.axes_needing_homing();
        LOOP_XY(axis) {
            // TODO: make homeaxis non-blocking to allow quick_stop
            if (is_park_axis[axis]) {
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
        }
#endif /*CORE_IS_XY*/

        // XY park (includes dock avoidance on INDX); mapi::park maps Unchanged to current_position
        log_info(MarlinServer, "Parking XY");
        mapi::park(xy_target.without_z_move());
    }

    report_current_position();
}

void Pause::unpark_nozzle_and_notify() {
#if HAS_CRASH_DETECTION()
    if (crash_s.get_state() == Crash_s::RECOVERY) {
        // With the gcode_interrupt mechanism, gcodes can get executed during the crash recovery process (typically M600 during runout).
        // In that case, we don't unpark. We just return the extruder to the same position we found it in (happens at the end of filament change)
        // and crash recovery handles unparking
        return;
    }
#endif // HAS_CRASH_DETECTION()

    if (isnan(settings.resume_pos.x) || isnan(settings.resume_pos.y) || isnan(settings.resume_pos.z)) {
        return;
    }

    setPhase(PhasesLoadUnload::Unparking);
    // Move XY to starting position, then Z
    const bool x_greater_than_y = parkMoveXGreaterThanY(current_position.xyz(), settings.resume_pos.xyz());
    const float &begin_pos = x_greater_than_y ? current_position.x : current_position.y;
    const float &end_pos = x_greater_than_y ? settings.resume_pos.x : settings.resume_pos.y;

    const float Z_len = current_position.z - settings.resume_pos.z; // sign does not matter, does not check Z max val (unlike park_nozzle_and_notify)
    const float XY_len = begin_pos - end_pos; // sign does not matter

    // Home axes that were parked — needed if we moved only one axis during parking
    const xyz_bool_t park_axes = settings.park_point.axes_needing_homing();
    GcodeSuite::G28_no_parser(park_axes.x, park_axes.y, false,
        {
            .only_if_needed = true,
            .z_raise = 0,
            .precise = false, // We don't need precise position for this procedure
        });

    {
        PauseFsmExplicitProgressNotifier N(*this, begin_pos, end_pos, 0, parkMoveXYPercent(Z_len, XY_len), marlin_vars().native_pos[x_greater_than_y ? MARLIN_VAR_INDEX_X : MARLIN_VAR_INDEX_Y]);
        mapi::park({ .x = settings.resume_pos.x, .y = settings.resume_pos.y });
    }

    // Move Z_AXIS to saved position, scope for PauseFsmNotifier
    {
        PauseFsmExplicitProgressNotifier N(*this, current_position.z, settings.resume_pos.z, parkMoveXYPercent(Z_len, XY_len), 100, marlin_vars().native_pos[MARLIN_VAR_INDEX_Z]); // from XY% to 100%

        // FIXME: use a beter movement api when available
        do_blocking_move_to_z(settings.resume_pos.z, feedRate_t(NOZZLE_PARK_Z_FEEDRATE), Segmented::yes);
    }

    // Unretract
    if (std::abs(settings.retract) > 1e-6f) {
        plan_e_move(-settings.retract, standard_feedrates::extruder(standard_feedrates::Extruder::deretract, FilamentType::for_current_tool_heuristic()));
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

    if (!DEBUGGING(DRYRUN) && settings.unload_length && thermalManager.targetTooColdToExtrude(settings.virtual_tool().to_physical())) {
        SERIAL_ECHO_MSG(MSG_ERR_HOTEND_TOO_COLD);
        return; // unable to reach safe temperature
    }

    // Lock filament sensor, so it does not enqueue new M600, beacuse of filament run out
    FS_EventAutolock runout_disable;

    // Indicate that the printer is paused
    ++did_pause_print;

    print_job_timer.pause();

    // Save print speed - M600 bypasses the pause state machine (pause_print/resume),
    // so feedrate_percentage must be saved and restored here explicitly.
    const int16_t saved_feedrate_percentage = feedrate_percentage;

    // Wait for buffered blocks to complete
    planner.synchronize();

    float initial_retracted_distance = 0;

#if HAS_FILAMENT_TRACKER()
    // Snapshot the current retraction, so the filament can be put back to it at the end
    const auto tool = PhysicalToolIndex::currently_selected_opt();
    if (tool.has_value()) {
        initial_retracted_distance = buddy::filament_tracker().get_retracted_distance(*tool).value_or(0);
    }
#endif

    invoke_loop();

    // Now all extrusion positions are resumed and ready to be confirmed
    // Put the filament back to the retraction it had when the filament change started;
    mapi::restore_retracted_distance(initial_retracted_distance, standard_feedrates::current_extruder(standard_feedrates::Extruder::retract));

    sync_e_position_to(settings.resume_pos.e);
    destination.e = settings.resume_pos.e;

    feedrate_percentage = saved_feedrate_percentage;

    --did_pause_print;

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

    const auto virtual_tool = stdext::get_optional<VirtualToolIndex>(VirtualToolIndex::currently_selected());
    if (!virtual_tool) {
        return false;
    }

    const RammingSequence *ramming_sequence = nullptr;

    switch (load_type) {
    case LoadType::filament_change:
    case LoadType::filament_stuck:
#if HAS_SPOOL_JOIN() && HAS_TOOLCHANGER()
    case LoadType::unload_spool_join:
#endif
        ramming_sequence = &standard_ramming_sequence(StandardRammingSequence::runout, *virtual_tool);
        break;

    default:
        ramming_sequence = &standard_ramming_sequence(StandardRammingSequence::unload, *virtual_tool);
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
    std::ignore = do_e_move_notify_progress_coldextrude(
        -remaining_unload_length, standard_feedrates::extruder(standard_feedrates::Extruder::filament_unload, FilamentType::for_tool_heuristic(settings.virtual_tool())), StopConditions::UserStopped);

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
    const bool filament_surely_removed = FSensors_instance().no_filament_surely(LogicalFilamentSensor::extruder)
        || (!FSensors_instance().sensor(LogicalFilamentSensor::extruder) && FSensors_instance().no_filament_surely(LogicalFilamentSensor::side));
    if (filament_surely_removed) {
        set(state_to_set);
        config_store().set_filament_type(settings.virtual_tool(), FilamentType::none);
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
#if HAS_SPOOL_JOIN() && HAS_TOOLCHANGER()
    case LoadType::unload_spool_join:
#endif
    case LoadType::filament_stuck: {
        constexpr static ProgressMapperWorkflowArray workflow { std::to_array<WorkflowStep>({
            { LoadState::unload_wait_temp, 3 },
#if HAS_INDX()
                { LoadState::unload_purge, 1 },
#endif
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
#if HAS_INDX()
                { LoadState::unload_purge, 1 },
#endif
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
    original_print_fan_speed = thermalManager.get_print_fan_speed();
    thermalManager.set_print_fan_speed(0);
}

Pause::FSM_HolderLoadUnload::~FSM_HolderLoadUnload() {
    active = false;

    restore_temperature_and_unpark();

    // Only now, so the heatup above does not have to fight the part cooling
    thermalManager.set_print_fan_speed(original_print_fan_speed);

    // Must run on every path out of the above: this is the only place the load/unload
    // mode is ever cleared, and a mode left set makes prusa_toolchanger.loop() skip
    // its "tool fell off" check for the rest of the boot.
    pause.clr_mode();
}

void Pause::FSM_HolderLoadUnload::restore_temperature_and_unpark() {
    if (!marlin_client::is_printing() && !marlin_client::is_paused()) {
        return;
    }

    // Restore the print temperature and wait for the heatup, so we resume fully heated
    // instead of at the filament default that loading dropped the target to. After a
    // runout the head already sits at the park height, so the unpark below is skipped -
    // which used to skip this along with it.
    if (const auto temp = pause.settings.resume_nozzle_temperature) {
        thermalManager.setTargetHotend(*temp, pause.settings.physical_tool());
        if (!pause.ensureSafeTemperatureNotifyProgress()) {
            return;
        }
    }

    {
        // Not made redundant by the heatup above: M701/M702 resuming a paused print set a
        // resume point but never a resume temperature, so this is their only heatup wait.
        if (!pause.ensureSafeTemperatureNotifyProgress()) {
            return;
        }
        pause.unpark_nozzle_and_notify();
    }
}

bool Pause::FSM_HolderLoadUnload::active = false;
