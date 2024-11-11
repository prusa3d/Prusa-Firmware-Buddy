/**
 * @file pause.cpp
 * @author Radek Vana
 * @brief stubbed version of marlin pause.cpp
 * mainly used for load / unload / change filament
 * @date 2020-12-18
 */

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

#if ENABLED(FWRETRACT)
    #include "fwretract.h"
#endif

#include "Marlin/src/lcd/extensible_ui/ui_api.h"
#include "Marlin/src/core/language.h"
#include "Marlin/src/lcd/ultralcd.h"

#include "Marlin/src/libs/nozzle.h"
#include "Marlin/src/feature/pause.h"
#include "filament_sensors_handler.hpp"
#include "pause_stubbed.hpp"
#include "safety_timer_stubbed.hpp"
#include "marlin_server.hpp"
#include "fs_event_autolock.hpp"
#include "filament.hpp"
#include "client_response.hpp"
#include "fsm_loadunload_type.hpp"
#include "RAII.hpp"
#include "mapi/motion.hpp"
#include <cmath>
#include <config_store/store_instance.hpp>
#include <scope_guard.hpp>
#include <filament_to_load.hpp>
#include <common/marlin_client.hpp>
#include <common/mapi/parking.hpp>

#include <option/has_human_interactions.h>
#include <option/has_mmu2.h>
#include <option/has_wastebin.h>

#ifndef NOZZLE_UNPARK_XY_FEEDRATE
    #define NOZZLE_UNPARK_XY_FEEDRATE NOZZLE_PARK_XY_FEEDRATE
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
    HAS_LCD_MENU || \
    NUM_RUNOUT_SENSORS > 1 || \
    ENABLED(DUAL_X_CARRIAGE) || \
    ENABLED(ADVANCED_PAUSE_CONTINUOUS_PURGE) || \
    BOTH(FILAMENT_UNLOAD_ALL_EXTRUDERS, MIXING_EXTRUDER) || \
    ENABLED(SDSUPPORT)
#error unsupported
#endif
// clang-format on

class PauseFsmNotifier : public marlin_server::FSM_notifier {
    Pause &pause;

public:
    PauseFsmNotifier(Pause &p, float min, float max, uint8_t progress_min, uint8_t progress_max, const MarlinVariable<float> &var_id)
        : FSM_notifier(ClientFSM::Load_unload, p.getPhaseIndex(), min, max, progress_min, progress_max, var_id)
        , pause(p) {}

    virtual fsm::PhaseData serialize(uint8_t progress) override {
        std::optional<LoadUnloadMode> mode = pause.get_mode();
        if (mode) {
            ProgressSerializerLoadUnload serializer(*mode, progress);
            return serializer.Serialize();
        }

        assert("unknown LoadUnloadMode");
        return { {} };
    }
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

class RammingSequence {
public:
    struct Item {
        int16_t e; ///< relative movement of Extruder
        int16_t feedrate; ///< feedrate of the move
    };

    template <size_t elements>
    constexpr RammingSequence(const Item (&sequence)[elements])
        : m_begin(sequence)
        , m_end(&(sequence[elements]))
        , unload_length(calculateRamUnloadLen(sequence)) {}
    const Item *begin() const { return m_begin; }
    const Item *end() const { return m_end; }

private:
    const Item *const m_begin;
    const Item *const m_end;

public:
    const decltype(Item::e) unload_length;

private:
    template <size_t elements>
    static constexpr decltype(Item::e) calculateRamUnloadLen(const Item (&ramSeq)[elements]) {
        decltype(Item::e) ramLen = 0;
        size_t i = 0;
        // skip all extrude movements
        while (i < elements && ramSeq[i].e > 0) {
            i++;
        }
        // calculate ram movement len
        for (; i < elements; i++) {
            ramLen += ramSeq[i].e;
        }
        return ramLen;
    }
};

#ifdef FILAMENT_RUNOUT_RAMMING_SEQUENCE
constexpr RammingSequence::Item ramRunoutSeqItems[] = FILAMENT_RUNOUT_RAMMING_SEQUENCE;
#else
constexpr RammingSequence::Item ramRunoutSeqItems[] = FILAMENT_UNLOAD_RAMMING_SEQUENCE;
#endif
constexpr RammingSequence runoutRammingSequence = RammingSequence(ramRunoutSeqItems);

constexpr RammingSequence::Item ramUnloadSeqItems[] = FILAMENT_UNLOAD_RAMMING_SEQUENCE;
constexpr RammingSequence unloadRammingSequence = RammingSequence(ramUnloadSeqItems);

/*****************************************************************************/
// PausePrivatePhase

PausePrivatePhase::PausePrivatePhase()
    : phase(PhasesLoadUnload::initial)
    , bed_restore_temp(NAN) {
    HOTEND_LOOP() {
        nozzle_restore_temp[e] = NAN;
    }
}

void PausePrivatePhase::setPhase(PhasesLoadUnload ph, uint8_t progress) {
    phase = ph;
    if (load_unload_mode) {
        ProgressSerializerLoadUnload serializer(*load_unload_mode, progress);
        marlin_server::fsm_change(phase, serializer.Serialize());
    }
}

PhasesLoadUnload PausePrivatePhase::getPhase() const { return phase; }

Response PausePrivatePhase::getResponse() {
    const Response ret = marlin_server::get_response_from_phase(phase);
    // user just clicked
    if (ret != Response::_none) {
        RestoreTemp();
    }
    return ret;
}

bool PausePrivatePhase::CanSafetyTimerExpire() const {
    if (HasTempToRestore()) {
        return false; // already expired
    }
    if ((getPhase() == PhasesLoadUnload::MakeSureInserted_stoppable) || (getPhase() == PhasesLoadUnload::MakeSureInserted_unstoppable) || (getPhase() == PhasesLoadUnload::FilamentNotInFS)) { // special waiting state without button
        return true; // waits for filament sensor
    }
    return ClientResponses::HasButton(getPhase()); // button in current phase == can wait on user == can timeout
}

void PausePrivatePhase::NotifyExpiredFromSafetyTimer() {
    if (CanSafetyTimerExpire()) {
        HOTEND_LOOP() {
            nozzle_restore_temp[e] = thermalManager.degTargetHotend(e);
        }
        bed_restore_temp = thermalManager.degTargetBed();
    }
}

void PausePrivatePhase::clrRestoreTemp() {
    HOTEND_LOOP() {
        nozzle_restore_temp[e] = NAN;
    }
    bed_restore_temp = NAN;
}

void PausePrivatePhase::RestoreTemp() {
    HOTEND_LOOP() {
        if (!isnan(nozzle_restore_temp[e])) {
            thermalManager.setTargetHotend(nozzle_restore_temp[e], e);
            marlin_server::set_temp_to_display(nozzle_restore_temp[e], e);
            nozzle_restore_temp[e] = NAN;
        }
    }
    if (!isnan(bed_restore_temp)) {
        thermalManager.setTargetBed(bed_restore_temp);
        bed_restore_temp = NAN;
    }
}

bool PausePrivatePhase::HasTempToRestore() const {
    HOTEND_LOOP() {
        if (!isnan(nozzle_restore_temp[e])) {
            return true;
        }
    }
    return !isnan(bed_restore_temp);
}

/*****************************************************************************/
// Pause
Pause &Pause::Instance() {
    static Pause s;
    return s;
}

bool Pause::is_unstoppable() {
    switch (load_type) {
    case LoadType::load:
        return FSensors_instance().HasMMU();
    case LoadType::load_to_gears:
        return !option::has_human_interactions;
    case LoadType::non_blocking_load:
    case LoadType::filament_change:
    case LoadType::filament_stuck:
        return true;
    default:
        return false;
    }
}

Pause::LoadState Pause::get_start_state() {
    switch (load_type) {
    case LoadType::load:
    case LoadType::autoload:
    case LoadType::load_to_gears:
    case LoadType::non_blocking_load:
    case LoadType::load_purge:
        return LoadState::load_init;
    case LoadType::unload:
    case LoadType::unload_confirm:
    case LoadType::unload_from_gears:
    case LoadType::filament_change:
    case LoadType::filament_stuck:
        return LoadState::unload_init;
    }

    bsod("Unhandled LoadType");
}

LoadUnloadMode Pause::get_load_unload_mode() {
    switch (load_type) {
    case Pause::LoadType::load:
    case Pause::LoadType::autoload:
    case Pause::LoadType::load_to_gears:
    case Pause::LoadType::non_blocking_load:
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
    case Pause::LoadType::autoload:
    case Pause::LoadType::load_purge:
        return false;
    case Pause::LoadType::load_to_gears:
    case Pause::LoadType::non_blocking_load:
        return !FSensors_instance().has_filament_surely();
    default:
        return true;
    }
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

    if (Temperature::degHotend(active_extruder) + heating_phase_min_hotend_diff > Temperature::degTargetHotend(active_extruder)) { // do not disturb user with heating dialog
        return true;
    }

    setPhase(settings.can_stop ? PhasesLoadUnload::WaitingTemp_stoppable : PhasesLoadUnload::WaitingTemp_unstoppable, progress_min);

    PauseFsmNotifier N(*this, Temperature::degHotend(active_extruder),
        Temperature::degTargetHotend(active_extruder) - heating_phase_min_hotend_diff, progress_min, progress_max, marlin_vars().hotend(active_extruder).temp_nozzle);

    // Wait until temperature is close
    while (Temperature::degHotend(active_extruder) < (Temperature::degTargetHotend(active_extruder) - heating_phase_min_hotend_diff)) {
        if (check_user_stop()) {
            return false;
        }
        idle(true, true);
    }

    // Check that safety timer didn't disable heaters
    if (Temperature::degTargetHotend(active_extruder) == 0) {
        return false;
    }

    return true;
}

void Pause::do_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max) {
    PauseFsmNotifier N(*this, current_position.e, current_position.e + length, progress_min, progress_max, marlin_vars().native_pos[MARLIN_VAR_INDEX_E]);

    mapi::extruder_move(length, fr_mm_s);

    wait_for_motion_finish_or_user_stop();
}

void Pause::do_e_move_notify_progress_coldextrude(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max) {
#if ENABLED(PREVENT_COLD_EXTRUSION)
    AutoRestore<bool> CE(thermalManager.allow_cold_extrude);
    thermalManager.allow_cold_extrude = true;
#endif
    do_e_move_notify_progress(length, fr_mm_s, progress_min, progress_max);
}

void Pause::do_e_move_notify_progress_hotextrude(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max) {
    PhasesLoadUnload last_ph = getPhase();

    if (!ensureSafeTemperatureNotifyProgress(0, 100)) {
        return;
    }

    // Restore phase after possible heatup GUI
    setPhase(last_ph, progress_min);

    do_e_move_notify_progress(length, fr_mm_s, progress_min, progress_max);
}

void Pause::plan_e_move(const float &length, const feedRate_t &fr_mm_s) {
    // It is unclear why this is one of the very few places where planner.buffer_line result is checked.
    // The draining condition is supposed to end the loop on power panic or crash.
    while (!mapi::extruder_move(length, fr_mm_s) && !planner.draining()) {
        delay(50);
    }
}

bool Pause::process_stop() {
    if (planner.draining()) { // Planner is draining, someone stopped, stop load/unload as well
        settings.do_stop = false;
        return true;
    }

    if (!settings.do_stop) {
        return false;
    }

    settings.do_stop = false;
    return true;
}

void Pause::set_unload_next_phase() {
    if (load_type == LoadType::filament_change || load_type == LoadType::filament_stuck) {
        set(LoadState::load_init);
    } else {
        set(LoadState::_finish);
    }
}

void Pause::load_init_process([[maybe_unused]] Response response) {
    // TODO: this shouldn't be needed here
    // actual temperature does not matter, only target
    if (!is_target_temperature_safe() && load_type != LoadType::load_to_gears) {
        settings.do_stop = true;
        return;
    }

#if HAS_MMU2()
    if (FSensors_instance().HasMMU()) {
        if (load_type == LoadType::load) {
            if (!MMU2::mmu2.load_filament_to_nozzle(settings.mmu_filament_to_load)) {
                // TODO tell user that he has already loaded filament if he really wants to continue
                // TODO check fsensor .. how should I behave if filament is not detected ???
                // some error?
                set(LoadState::load_finish);
                return;
            }

            config_store().set_filament_type(settings.GetExtruder(), filament::get_type_to_load());

            setPhase(PhasesLoadUnload::IsColor, 99);
            set(LoadState::color_correct_ask);
        } else if (load_type == LoadType::filament_change) {
            if (settings.mmu_filament_to_load == MMU2::FILAMENT_UNKNOWN) {
                set(LoadState::load_finish);
                return;
            }

            setPhase(PhasesLoadUnload::LoadFilamentIntoMMU);
            set(LoadState::mmu_load_ask);
        }

        return;
    }
#endif

    switch (load_type) {
    case LoadType::load_to_gears:
#if HAS_SIDE_FSENSOR()
        set_timed(LoadState::assist_insertion);
        break;
#else
        set(LoadState::load_to_gears);
#endif
        break;
    case LoadType::non_blocking_load:
        // If we are certain that filament has been inserted, we can skip after load_to_gears phase
        if (FSensors_instance().has_filament_surely()) {
            set(LoadState::move_to_purge);
        } else if constexpr (!option::has_side_fsensor) {
            set(LoadState::load_to_gears);
#if HAS_SIDE_FSENSOR()
        } else {
            set_timed(LoadState::await_filament);
#endif
        }
        break;
    case LoadType::autoload:
        // if filament is not present we want to break and not set loaded filament
        // we have already loaded the filament in gear, now just wait for temperature to rise
        config_store().set_filament_type(settings.GetExtruder(), filament::get_type_to_load());
        set(LoadState::wait_temp);
        handle_filament_removal(LoadState::filament_push_ask);
        break;
    case LoadType::load_purge:
        set(LoadState::wait_temp);
        break;

    default:
#if HAS_SIDE_FSENSOR()
        set_timed(LoadState::await_filament);
#else
        set(LoadState::filament_push_ask);
#endif
        break;
    }
}

void Pause::filament_push_ask_process(Response response) {
    if constexpr (!option::has_human_interactions) {
        setPhase(PhasesLoadUnload::Inserting_unstoppable);

        if (FSensors_instance().has_filament_surely()) {
            set(LoadState::move_to_purge);
#if HAS_SIDE_FSENSOR()
        } else if constexpr (option::has_side_fsensor) {
            set_timed(LoadState::await_filament);
#endif
        } else {
            set(LoadState::load_to_gears);
        }

        return;
    }

    if (FSensors_instance().no_filament_surely()) {
        setPhase(is_unstoppable() ? PhasesLoadUnload::MakeSureInserted_unstoppable : PhasesLoadUnload::MakeSureInserted_stoppable);

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

        if (response == Response::Continue || settings.extruder_mmu_rework) {

            if (FSensors_instance().has_filament_surely()) {
                set(LoadState::load_to_gears);
#if HAS_SIDE_FSENSOR()
            } else if constexpr (option::has_side_fsensor) {
                set_timed(LoadState::await_filament);
#endif
            }
        }
    }

    if (response == Response::Stop) {
        settings.do_stop = true;
    }
}

#if HAS_SIDE_FSENSOR()
void Pause::await_filament_process([[maybe_unused]] Response response) {
    setPhase(PhasesLoadUnload::Inserting_stoppable);
    // If EXTRUDER sensor is not assigned or not working, or if the user fails to insert filament in time, show Warning and quit loading.
    if (!FSensors_instance().sensor(LogicalFilamentSensor::current_extruder) || !is_fsensor_working_state(FSensors_instance().sensor_state(LogicalFilamentSensor::current_extruder)) || ticks_diff(ticks_ms(), start_time_ms) > 10 * 60 * 1000) {
        marlin_server::set_warning(WarningType::FilamentLoadingTimeout);
        set(LoadState::_finish);
        return;
    }

    if (!FSensors_instance().sensor(LogicalFilamentSensor::current_side) || !is_fsensor_working_state(FSensors_instance().sensor_state(LogicalFilamentSensor::current_side)) // If SIDE sensor is not assigned or not working go directly to loading
        || FSensors_instance().sensor(LogicalFilamentSensor::current_side)->get_state() == FilamentSensorState::HasFilament) { // If filament arrived at SIDE sensor, we can start loading sequence.
        set_timed(LoadState::assist_insertion);
        return;
    }
}

void Pause::assist_insertion_process([[maybe_unused]] Response response) {
    setPhase(is_unstoppable() ? PhasesLoadUnload::Inserting_unstoppable : PhasesLoadUnload::Inserting_stoppable, 10);

    // Filament is in Extruder autoload assistance si done.
    if (FSensors_instance().has_filament_surely()) {
        set(LoadState::load_to_gears);
        return;
    }

    // Moves are planned wait until they aren't before panning more
    if (planner.processing()) {
        return;
    }

    // Load for at least 40 seconds before giving up. Alternatively, if filament is removed altogether, stop too.
    auto sensor = FSensors_instance().sensor(LogicalFilamentSensor::current_side);
    if (ticks_diff(ticks_ms(), start_time_ms) > 40000 /*Move for at least 40 seconds before giving up*/
        || (sensor && is_fsensor_working_state(sensor->get_state()) && sensor->get_state() != FilamentSensorState::HasFilament)) {
        settings.do_stop = true;
        return;
    }

    do_e_move_notify_progress_coldextrude(FILAMENT_CHANGE_SLOW_LOAD_FEEDRATE, FILAMENT_CHANGE_SLOW_LOAD_FEEDRATE, 10, 10);
}
#endif

void Pause::load_to_gears_process([[maybe_unused]] Response response) { // slow load
    setPhase(is_unstoppable() ? PhasesLoadUnload::Inserting_unstoppable : PhasesLoadUnload::Inserting_stoppable, 10);

    do_e_move_notify_progress_coldextrude(settings.slow_load_length, FILAMENT_CHANGE_SLOW_LOAD_FEEDRATE, 10, 30); // TODO method without param using actual phase

    // if filament is not present we want to break and not set loaded filament
    set(LoadState::move_to_purge);
    handle_filament_removal(LoadState::filament_push_ask);
}

void Pause::move_to_purge_process([[maybe_unused]] Response response) {
    if constexpr (option::has_side_fsensor) {
        mapi::park_move_with_conditional_home(mapi::park_positions[mapi::ParkPosition::purge], mapi::ZAction::no_move);
    }
    if (load_type == LoadType::load_to_gears) {
        set(LoadState::_finish);
    } else {
        set(LoadState::wait_temp);
    }
}

void Pause::wait_temp_process([[maybe_unused]] Response response) {
    if (ensureSafeTemperatureNotifyProgress(30, 50)) {
        if (load_type == LoadType::load_purge) {
            set(LoadState::purge);
        } else {
            set(LoadState::long_load);
        }
    }
    handle_filament_removal(LoadState::filament_push_ask);
}

void Pause::long_load_process([[maybe_unused]] Response response) {
    setPhase(is_unstoppable() ? PhasesLoadUnload::Loading_unstoppable : PhasesLoadUnload::Loading_stoppable, 50);

    const float saved_acceleration = planner.user_settings.retract_acceleration;
    {
        auto s = planner.user_settings;
        s.retract_acceleration = FILAMENT_CHANGE_FAST_LOAD_ACCEL;
        planner.apply_settings(s);
    }

    do_e_move_notify_progress_hotextrude(settings.fast_load_length, FILAMENT_CHANGE_FAST_LOAD_FEEDRATE, 50, 70);

    {
        auto s = planner.user_settings;
        s.retract_acceleration = saved_acceleration;
        planner.apply_settings(s);
    }

    set(LoadState::purge);
    handle_filament_removal(LoadState::filament_push_ask);
}

void Pause::purge_process([[maybe_unused]] Response response) {
    // Extrude filament to get into hotend
    setPhase(is_unstoppable() ? PhasesLoadUnload::Purging_unstoppable : PhasesLoadUnload::Purging_stoppable, 70);
    do_e_move_notify_progress_hotextrude(settings.purge_length(), ADVANCED_PAUSE_PURGE_FEEDRATE, 70, 99);
    config_store().set_filament_type(settings.GetExtruder(), filament::get_type_to_load());

    if constexpr (!option::has_human_interactions) {
        set(LoadState::load_finish);
        return;
    }

    setPhase(PhasesLoadUnload::IsColor, 99);
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
        set(LoadState::load_finish);
        break;

    default:
        // This doesn't make sense with the MMU on
        if (!FSensors_instance().HasMMU()) {
            handle_filament_removal(LoadState::filament_push_ask);
        }
    }
}

#if HAS_MMU2()
void Pause::mmu_load_ask_process(Response response) {
    if (response == Response::Continue) {
        set(LoadState::mmu_load);
    }
}

void Pause::mmu_load_process([[maybe_unused]] Response response) {
    if (settings.mmu_filament_to_load == MMU2::FILAMENT_UNKNOWN) {
        set(LoadState::load_finish);
        return;
    }

    MMU2::mmu2.load_filament(settings.mmu_filament_to_load);
    MMU2::mmu2.load_filament_to_nozzle(settings.mmu_filament_to_load);

    setPhase(PhasesLoadUnload::IsColor, 99);
    set(LoadState::color_correct_ask);
}
#endif

void Pause::eject_process([[maybe_unused]] Response response) {
#if HAS_MMU2()
    if (FSensors_instance().HasMMU()) {
        MMU2::mmu2.unload();
        if (load_type == LoadType::filament_change) {
            set(LoadState::mmu_load);
        } else {
            set(LoadState::load_init);
        }
        return;
    }
#endif

    setPhase(is_unstoppable() ? PhasesLoadUnload::Ramming_unstoppable : PhasesLoadUnload::Ramming_stoppable, 98);
    ram_filament();

    planner.synchronize(); // do_pause_e_move(0, (FILAMENT_CHANGE_UNLOAD_FEEDRATE));//do previous moves, so Ramming text is visible

    setPhase(is_unstoppable() ? PhasesLoadUnload::Ejecting_unstoppable : PhasesLoadUnload::Ejecting_stoppable, 99);
    unload_filament();

    switch (load_type) {
    case LoadType::load_to_gears:
    case LoadType::non_blocking_load:
#if !HAS_HUMAN_INTERACTIONS()
        // This state should be unreachable for printers without Human interaction and could be unrecoverable. We need to finish the FSM in order to not block interactions from Connect.
        set(LoadState::_finish);
        break;
#endif
    case LoadType::filament_change:
    case LoadType::filament_stuck:
        set(LoadState::load_init);
        break;
    case LoadType::load:
    case LoadType::autoload:
        set(LoadState::filament_push_ask);
        break;
    default:
        break;
    }
}

void Pause::load_finish_process([[maybe_unused]] Response response) {
    if (load_type == LoadType::filament_change || load_type == LoadType::filament_stuck) {
        // Feed a little bit of filament to stabilize pressure in nozzle

        // Last poop after user clicked color - yes
        plan_e_move(5, 10);

        // Retract again, it will be unretracted at the end of unpark
        if (settings.retract) {
            plan_e_move(settings.retract, PAUSE_PARK_RETRACT_FEEDRATE);
        }

        planner.synchronize();
        delay(500);
    }

    set(LoadState::_finish);
}

void Pause::unload_init_process([[maybe_unused]] Response response) {
    // loop_unload_mmu has it's own preheating sequence, use that one for better progress reporting
    if (!(load_type == LoadType::unload && FSensors_instance().HasMMU()) && !ensureSafeTemperatureNotifyProgress(0, 50) && load_type != LoadType::unload_from_gears) {
        settings.do_stop = true;
        return;
    }

#if HAS_MMU2()
    if (FSensors_instance().HasMMU()) {
        if (load_type == LoadType::unload) {
            MMU2::mmu2.unload();
            set(LoadState::_finish);
        } else if (load_type == LoadType::filament_change) {
            settings.mmu_filament_to_load = MMU2::mmu2.get_current_tool();

            // No filament loaded in MMU, we can't continue, as we don't know what slot to load
            if (settings.mmu_filament_to_load == MMU2::FILAMENT_UNKNOWN) {
                set_unload_next_phase();
                return;
            }

            MMU2::mmu2.unload();
            MMU2::mmu2.eject_filament(settings.mmu_filament_to_load);
            set_unload_next_phase();
        }

        return;
    }
#endif

    switch (load_type) {

    case LoadType::filament_stuck:
#if HAS_LOADCELL()
        setPhase(PhasesLoadUnload::FilamentStuck);
        set(LoadState::filament_stuck_ask);
#else
        setPhase(PhasesLoadUnload::ManualUnload, 100);
        set(LoadState::manual_unload);
#endif
        break;

    case LoadType::unload_from_gears:
        set(LoadState::unload_from_gears);
        break;

    default:
        set(LoadState::ram_sequence);
        break;
    }
}

void Pause::filament_stuck_ask_process(Response response) {
    if (response == Response::Unload) {
        set(LoadState::ram_sequence);
    }
}

void Pause::ram_sequence_process([[maybe_unused]] Response response) {
    setPhase(is_unstoppable() ? PhasesLoadUnload::Ramming_unstoppable : PhasesLoadUnload::Ramming_stoppable, 50);
    ram_filament();
    set(LoadState::unload);
}

void Pause::unload_process([[maybe_unused]] Response response) {
    setPhase(is_unstoppable() ? PhasesLoadUnload::Unloading_unstoppable : PhasesLoadUnload::Unloading_stoppable, 51);
    unload_filament();
    if (settings.do_stop) {
        return;
    }

    config_store().set_filament_type(settings.GetExtruder(), FilamentType::none);

    switch (load_type) {
    case LoadType::unload:
#if HAS_HUMAN_INTERACTIONS()
        set_unload_next_phase();
        break;
#endif
    case LoadType::unload_confirm:
    case LoadType::filament_change:
    case LoadType::filament_stuck:
#if !HAS_HUMAN_INTERACTIONS()
        runout_timer_ms = ticks_ms();
        set(LoadState::filament_not_in_fs);
        break;
#endif

        setPhase(PhasesLoadUnload::IsFilamentUnloaded, 100);
        set(LoadState::unloaded_ask);
        break;
    default:
        break;
    }
}

void Pause::unloaded_ask_process(Response response) {
    if (response == Response::Yes) {
        set(LoadState::filament_not_in_fs);
        return;
    }
    if (response == Response::No) {
        setPhase(PhasesLoadUnload::ManualUnload, 100);
        disable_e_stepper(active_extruder);
        set(LoadState::manual_unload);
    }
}

void Pause::unload_from_gears_process([[maybe_unused]] Response response) {
    setPhase(PhasesLoadUnload::Unloading_stoppable, 0);
    do_e_move_notify_progress_coldextrude(-settings.slow_load_length * (float)1.5, FILAMENT_CHANGE_FAST_LOAD_FEEDRATE, 0, 100);
    set_unload_next_phase();
}

void Pause::filament_not_in_fs_process([[maybe_unused]] Response response) {
    setPhase(PhasesLoadUnload::FilamentNotInFS);
    if (!FSensors_instance().has_filament_surely()) { // Either no filament in FS or unknown (FS off)
#if !HAS_HUMAN_INTERACTIONS()
        // In case of no human interactions, require no filament being
        // detected for at least 1s to avoid FS flicking off and on due
        // to filament movement and reloading the just-unloaded
        // filament remnant back in.
        if (ticks_diff(ticks_ms(), runout_timer_ms) < 1000) {
            return;
        }
#endif
        set_unload_next_phase();
    } else {
#if !HAS_HUMAN_INTERACTIONS()
        runout_timer_ms = ticks_ms();
#endif
    }
}

void Pause::manual_unload_process(Response response) {
    if (response == Response::Continue
        && !FSensors_instance().has_filament_surely()) { // Allow to continue when nothing remains in filament sensor
        enable_e_steppers();
        set_unload_next_phase();
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

    set(get_start_state());

    bool ret = true;
    while (!finished()) {
        ret = !process_stop();
        if (ret) {
            (this->*(state_handlers[state]))(getResponse());
        } else {
            set(LoadState::_finish);
            continue;
        }
        ret = !process_stop(); // why is this 2nd call here ???, some workaround ???
        if (ret) {
            idle(true, true); // idle loop to prevent wdt and manage heaters etc, true == do not shutdown steppers
        } else {
            set(LoadState::_finish);
            continue;
        }
    };

#if ENABLED(PID_EXTRUSION_SCALING)
    thermalManager.setExtrusionScalingEnabled(extrusionScalingEnabled);
#endif // ENABLED(PID_EXTRUSION_SCALING)

    return ret;
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

bool Pause::wait_for_motion_finish_or_user_stop() {
    while (planner.processing()) {
        if (check_user_stop()) {
            return true;
        }
        idle(true, true);
    }
    return false;
}

void Pause::park_nozzle_and_notify() {
    setPhase(settings.can_stop ? PhasesLoadUnload::Parking_stoppable : PhasesLoadUnload::Parking_unstoppable);
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

    // move by z_lift, scope for PauseFsmNotifier
    if (isfinite(target_Z)) {
        if (axes_need_homing(_BV(Z_AXIS))) {
            unhomed_z_lift(target_Z);
        } else {
            PauseFsmNotifier N(*this, current_position.z, target_Z, 0, parkMoveZPercent(Z_len, XY_len), marlin_vars().native_pos[MARLIN_VAR_INDEX_Z]);
            plan_park_move_to(current_position.x, current_position.y, target_Z, NOZZLE_PARK_XY_FEEDRATE, Z_feedrate);
            if (wait_for_motion_finish_or_user_stop()) {
                return;
            }
        }
    }

    // move to (x_pos, y_pos)
    if (XY_len != 0) {
#if CORE_IS_XY
        if (axes_need_homing(_BV(X_AXIS) | _BV(Y_AXIS))) {
            GcodeSuite::G28_no_parser(false, 0, false, true, true, false);

            // We have moved both axes, go to park position if not requested otherwise
    #ifdef XYZ_NOZZLE_PARK_POINT_M600
            static constexpr xyz_pos_t park = XYZ_NOZZLE_PARK_POINT_M600;
    #else /*XYZ_NOZZLE_PARK_POINT_M600*/
            static constexpr xyz_pos_t park = XYZ_NOZZLE_PARK_POINT;
    #endif /*XYZ_NOZZLE_PARK_POINT_M600*/
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
            if (!isnan(settings.park_pos.pos[axis]) && axes_need_homing(_BV(axis))) {
                GcodeSuite::G28_no_parser(false, 0, false, axis == X_AXIS, axis == Y_AXIS, false);
            }
            if (check_user_stop()) {
                return;
            }
            if (isnan(settings.park_pos.pos[axis])) {
                settings.park_pos.pos[axis] = current_position.pos[axis];
            }
        }
#endif /*CORE_IS_XY*/

        if (x_greater_than_y) {
            PauseFsmNotifier N(*this, begin_pos, end_pos, parkMoveZPercent(Z_len, XY_len), 100, marlin_vars().native_pos[MARLIN_VAR_INDEX_X]); // from Z% to 100%
            plan_park_move_to_xyz(settings.park_pos, NOZZLE_PARK_XY_FEEDRATE, Z_feedrate);
            if (wait_for_motion_finish_or_user_stop()) {
                return;
            }
        } else {
            PauseFsmNotifier N(*this, begin_pos, end_pos, parkMoveZPercent(Z_len, XY_len), 100, marlin_vars().native_pos[MARLIN_VAR_INDEX_Y]); // from Z% to 100%
            plan_park_move_to_xyz(settings.park_pos, NOZZLE_PARK_XY_FEEDRATE, Z_feedrate);
            if (wait_for_motion_finish_or_user_stop()) {
                return;
            }
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
    LOOP_XY(axis) {
        if (!isnan(settings.park_pos.pos[axis]) && axes_need_homing(_BV(axis))) {
            GcodeSuite::G28_no_parser(false, 0, false, axis == X_AXIS, axis == Y_AXIS, false);
        }
    }

    if (x_greater_than_y) {
        PauseFsmNotifier N(*this, begin_pos, end_pos, 0, parkMoveXYPercent(Z_len, XY_len), marlin_vars().native_pos[MARLIN_VAR_INDEX_X]);
        do_blocking_move_to_xy(settings.resume_pos, NOZZLE_UNPARK_XY_FEEDRATE);
    } else {
        PauseFsmNotifier N(*this, begin_pos, end_pos, 0, parkMoveXYPercent(Z_len, XY_len), marlin_vars().native_pos[MARLIN_VAR_INDEX_Y]);
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

        PauseFsmNotifier N(*this, current_position.z, resume_pos_adj.z, parkMoveXYPercent(Z_len, XY_len), 100, marlin_vars().native_pos[MARLIN_VAR_INDEX_Z]); // from XY% to 100%
        // FIXME: use a beter movement api when available
        do_blocking_move_to_z(resume_pos_adj.z, feedRate_t(NOZZLE_PARK_Z_FEEDRATE));
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
 *   -  FWRETRACT: Recover/prime from the prior G10.
 * - Move the nozzle back to resume_position
 * - Sync the planner E to resume_position.e
 * - Send host action for resume, if configured
 * - Resume the current SD print job, if any
 */
void Pause::filament_change(const pause::Settings &settings_, bool is_filament_stuck) {
    settings = settings_;
    settings.can_stop = false;

    load_type = is_filament_stuck ? LoadType::filament_stuck : LoadType::filament_change;

    if (did_pause_print) {
        return; // already paused
    }

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

// Intelligent resuming
#if ENABLED(FWRETRACT)
    // If retracted before goto pause
    if (fwretract.retracted[active_extruder]) {
        do_pause_e_move(-fwretract.settings.retract_length, fwretract.settings.retract_feedrate_mm_s);
    }
#endif

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

#if HAS_DISPLAY
    ui.reset_status();
#endif
}
void Pause::ram_filament() {
    const RammingSequence &sequence = get_ramming_sequence();

    constexpr float mm_per_minute = 1 / 60.f;
    // ram filament
    for (auto &elem : sequence) {
        plan_e_move(elem.e, elem.feedrate * mm_per_minute);
        if (check_user_stop()) {
            return;
        }
    }
}

void Pause::unload_filament() {
    const RammingSequence &sequence = get_ramming_sequence();

    const float saved_acceleration = planner.user_settings.retract_acceleration;
    {
        auto s = planner.user_settings;
        s.retract_acceleration = FILAMENT_CHANGE_UNLOAD_ACCEL;
        planner.apply_settings(s);
    }

    // subtract the already performed extruder movement from the total unload length and ensure it is negative

    float remaining_unload_length = -(std::abs(settings.unload_length) - std::abs(sequence.unload_length));
    if (remaining_unload_length > .0f) {
        remaining_unload_length = .0f;
    }
    do_e_move_notify_progress_hotextrude(remaining_unload_length, (FILAMENT_CHANGE_UNLOAD_FEEDRATE), 51, 99);

    {
        auto s = planner.user_settings;
        s.retract_acceleration = saved_acceleration;
        planner.apply_settings(s);
    }
}
const RammingSequence &Pause::get_ramming_sequence() const {
    switch (load_type) {
    case LoadType::filament_change:
    case LoadType::filament_stuck:
        return runoutRammingSequence;
    default:
        return unloadRammingSequence;
    }
}

bool Pause::check_user_stop() {
    if (user_stop_pending) {
        return true;
    }

    const Response response = getResponse();
    if (response != Response::Stop) {
        return false;
    }

    settings.do_stop = true;
    planner.quick_stop();

    // unwind to main marlin loop and let it call finalize_user_stop()
    user_stop_pending = true;
    return true;
}

void Pause::finalize_user_stop() {
    if (!user_stop_pending) {
        // nothing to do (at all)
        return;
    }

    if (planner.processing()) {
        // nothing to do (yet)
        return;
    }

    user_stop_pending = false;

    planner.resume_queuing();
    set_all_unhomed();
    set_all_unknown();
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
    current_position = real_current_position;
    planner.set_position_mm(current_position);
}
void Pause::handle_filament_removal(LoadState state_to_set) {
    // only if there is no filament present and we are sure (FS on and sees no filament)
    if (FSensors_instance().no_filament_surely()) {
        set(state_to_set);
        config_store().set_filament_type(settings.GetExtruder(), FilamentType::none);
        return;
    }
    return;
}

/*****************************************************************************/
// Pause::FSM_HolderLoadUnload

void Pause::FSM_HolderLoadUnload::bindToSafetyTimer() {
    SafetyTimer::Instance().BindPause(pause);
}

void Pause::FSM_HolderLoadUnload::unbindFromSafetyTimer() {
    SafetyTimer::Instance().UnbindPause(pause);
}

Pause::FSM_HolderLoadUnload::FSM_HolderLoadUnload(Pause &p)
    : FSM_Holder(PhasesLoadUnload::initial)
    , pause(p) {
    pause.set_mode(pause.get_load_unload_mode());
    pause.clrRestoreTemp();
    bindToSafetyTimer();
    if (pause.should_park()) {
        pause.park_nozzle_and_notify();
    }
    active = true;
}

Pause::FSM_HolderLoadUnload::~FSM_HolderLoadUnload() {
    active = false;
    pause.RestoreTemp();

    const float min_layer_h = 0.05f;
    // do not unpark and wait for temp if not homed or z park len is 0
    if (!axes_need_homing() && pause.settings.resume_pos.z != NAN && std::abs(current_position.z - pause.settings.resume_pos.z) >= min_layer_h && (marlin_client::is_printing() || marlin_client::is_paused())) {
        if (!pause.ensureSafeTemperatureNotifyProgress(0, 100)) {
            return;
        }
        pause.unpark_nozzle_and_notify();
    }
    pause.clr_mode();
    unbindFromSafetyTimer(); // unbind must be last action, without it Pause cannot block safety timer
}

bool Pause::FSM_HolderLoadUnload::active = false;
