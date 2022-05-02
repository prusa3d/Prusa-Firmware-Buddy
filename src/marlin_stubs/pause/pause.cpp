/**
 * @file pause.cpp
 * @author Radek Vana
 * @brief stubbed version of marlin pause.cpp
 * mainly used for load / unload / change filament
 * @date 2020-12-18
 */

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
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "filament_sensor_api.hpp"
#include "pause_stubbed.hpp"
#include "safety_timer_stubbed.hpp"
#include "marlin_server.hpp"
#include "fs_event_autolock.hpp"
#include "filament.hpp"
#include "client_response.hpp"
#include "RAII.hpp"
#include <cmath>
#include <functional> // std::invoke

#ifndef NOZZLE_UNPARK_XY_FEEDRATE
    #define NOZZLE_UNPARK_XY_FEEDRATE NOZZLE_PARK_XY_FEEDRATE
#endif

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

//cannot be class member (externed in marlin and used by M240 and tool_change)
void do_pause_e_move(const float &length, const feedRate_t &fr_mm_s) {
    current_position.e += length / planner.e_factor[active_extruder];
    line_to_current_position(fr_mm_s);
    planner.synchronize();
}

class RammingSequence {
public:
    struct Item {
        int16_t e;        ///< relative movement of Extruder
        int16_t feedrate; ///< feedrate of the move
    };

    template <size_t elements>
    constexpr RammingSequence(const Item (&sequence)[elements])
        : m_begin(sequence)
        , m_end(&(sequence[elements]))
        , unload_length(calculateRamUnloadLen(sequence)) {}
    const Item *const begin() const { return m_begin; }
    const Item *const end() const { return m_end; }

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
        while (ramSeq[i].e > 0 && i < elements) {
            i++;
        }
        //calculate ram movement len
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
//PausePrivatePhase

PausePrivatePhase::PausePrivatePhase()
    : phase(PhasesLoadUnload::_first)
    , load_unload_shared_phase(int(UnloadPhases_t::_init))
    , nozzle_restore_temp(NAN)
    , bed_restore_temp(NAN) {}

void PausePrivatePhase::setPhase(PhasesLoadUnload ph, uint8_t progress) {
    phase = ph;
    ProgressSerializer serializer(progress);
    fsm_change(ClientFSM::Load_unload, phase, serializer.Serialize());
}

PhasesLoadUnload PausePrivatePhase::getPhase() const { return phase; }

Response PausePrivatePhase::getResponse() {
    const Response ret = ClientResponseHandler::GetResponseFromPhase(phase);
    //user just clicked
    if (ret != Response::_none) {
        RestoreTemp();
    }
    return ret;
}

bool PausePrivatePhase::CanSafetyTimerExpire() const {
    if (HasTempToRestore())
        return false;                                                                                                                   // already expired
    if ((getPhase() == PhasesLoadUnload::MakeSureInserted_stoppable) || (getPhase() == PhasesLoadUnload::MakeSureInserted_unstoppable)) // special waiting state without button
        return true;                                                                                                                    // waits for filament sensor
    return ClientResponses::HasButton(getPhase());                                                                                      // button in current phase == can wait on user == can timeout
}

void PausePrivatePhase::NotifyExpiredFromSafetyTimer(float hotend_temp, float bed_temp) {
    if (CanSafetyTimerExpire()) {
        nozzle_restore_temp = hotend_temp;
        bed_restore_temp = bed_temp;
    }
}

void PausePrivatePhase::clrRestoreTemp() {
    nozzle_restore_temp = NAN;
    bed_restore_temp = NAN;
}

void PausePrivatePhase::RestoreTemp() {
    if (!isnan(nozzle_restore_temp)) {
        thermalManager.setTargetHotend(nozzle_restore_temp, 0);
        marlin_server_set_temp_to_display(nozzle_restore_temp);
        nozzle_restore_temp = NAN;
    }
    if (!isnan(bed_restore_temp)) {
        thermalManager.setTargetBed(bed_restore_temp);
        bed_restore_temp = NAN;
    }
}

bool PausePrivatePhase::HasTempToRestore() const {
    return (!isnan(nozzle_restore_temp)) || (!isnan(bed_restore_temp));
}

/*****************************************************************************/
//Pause
Pause &Pause::Instance() {
    static Pause s;
    return s;
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

    setPhase(settings.can_stop ? PhasesLoadUnload::WaitingTemp_stoppable : PhasesLoadUnload::WaitingTemp_unstoppable, progress_min);

    Notifier_TEMP_NOZ N(ClientFSM::Load_unload, getPhaseIndex(), Temperature::degHotend(active_extruder),
        Temperature::degTargetHotend(active_extruder), progress_min, progress_max);

    can_stop_wait_for_heatup(true);
    bool res = thermalManager.wait_for_hotend(active_extruder);
    can_stop_wait_for_heatup(false);
    if (!wait_for_heatup) {
        settings.do_stop = true;
        wait_for_heatup = true;
    }

    return res;
}

void Pause::do_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max) {
    //Not sure if following code would not be better
    //const float actual_e = planner.get_axis_position_mm(E_AXIS);
    //Notifier_POS_E N(ClientFSM::Load_unload, getPhaseIndex(), actual_e, actual_e + length, progress_min,progress_max);
    const float actual_e = current_position.e;
    current_position.e += length / planner.e_factor[active_extruder];
    Notifier_POS_E N(ClientFSM::Load_unload, getPhaseIndex(), actual_e, current_position.e, progress_min, progress_max);
    line_to_current_position(fr_mm_s);
    wait_or_stop();
}

void Pause::do_e_move_notify_progress_coldextrude(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max) {
    AutoRestore<bool> CE(thermalManager.allow_cold_extrude);
    thermalManager.allow_cold_extrude = true;
    do_e_move_notify_progress(length, fr_mm_s, progress_min, progress_max);
}

void Pause::do_e_move_notify_progress_hotextrude(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max) {
    PhasesLoadUnload last_ph = getPhase();
    if (!ensureSafeTemperatureNotifyProgress(0, 100))
        return;
    setPhase(last_ph, progress_min);
    do_e_move_notify_progress(length, fr_mm_s, progress_min, progress_max);
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
    while (!settings.do_stop && !planner.buffer_line(current_position, fr_mm_s, active_extruder)) {
        check_user_stop();
        delay(50);
    }
}

bool Pause::process_stop() {
    if (!settings.do_stop)
        return false;

    settings.do_stop = false;
    return true;
}

void Pause::loop_load(Response response) {
    const float purge_ln = settings.purge_length;

    // transitions
    switch (getLoadPhase()) {
    case LoadPhases_t::_init:
        set(LoadPhases_t::check_filament_sensor_and_user_push__ask);
        break;
    case LoadPhases_t::check_filament_sensor_and_user_push__ask:
        if (FSensors_instance().HasNotFilament()) {
            setPhase(PhasesLoadUnload::MakeSureInserted_stoppable);
        } else {
            setPhase(PhasesLoadUnload::UserPush_stoppable);
            if (response == Response::Continue) {
                set(LoadPhases_t::load_in_gear);
            }
        }
        if (response == Response::Stop)
            settings.do_stop = true;
        break;
    case LoadPhases_t::load_in_gear: //slow load
        setPhase(PhasesLoadUnload::Inserting_stoppable, 10);
        do_e_move_notify_progress_coldextrude(settings.slow_load_length, FILAMENT_CHANGE_SLOW_LOAD_FEEDRATE, 10, 30); // TODO method without param using actual phase
        Filaments::Set(Filaments::GetToBeLoaded());
        set(LoadPhases_t::wait_temp);
        break;
    case LoadPhases_t::wait_temp:
        if (ensureSafeTemperatureNotifyProgress(30, 50)) {
            set(LoadPhases_t::long_load);
        }
        break;
    case LoadPhases_t::error_temp:
        set(LoadPhases_t::_finish);
        break;
    case LoadPhases_t::long_load:
        planner.settings.retract_acceleration = FILAMENT_CHANGE_FAST_LOAD_ACCEL;
        setPhase(PhasesLoadUnload::Loading_stoppable, 50);
        do_e_move_notify_progress_hotextrude(settings.fast_load_length, FILAMENT_CHANGE_FAST_LOAD_FEEDRATE, 50, 70);
        set(LoadPhases_t::purge);
        break;
    case LoadPhases_t::purge:
        // Extrude filament to get into hotend
        setPhase(PhasesLoadUnload::Purging_stoppable, 70);
        do_e_move_notify_progress_hotextrude(purge_ln, ADVANCED_PAUSE_PURGE_FEEDRATE, 70, 99);
        setPhase(PhasesLoadUnload::IsColor, 99);
        set(LoadPhases_t::ask_is_color_correct);
        break;
    case LoadPhases_t::ask_is_color_correct:
        if (response == Response::Purge_more) {
            set(LoadPhases_t::purge);
        }
        if (response == Response::Retry) {
            set(LoadPhases_t::eject);
        }
        if (response == Response::Continue) {
            set(LoadPhases_t::_finish);
        }
        break;
    case LoadPhases_t::eject:
        setPhase(PhasesLoadUnload::Ramming_stoppable, 98);
        ram_filament(RammingType::unload);

        planner.synchronize(); // do_pause_e_move(0, (FILAMENT_CHANGE_UNLOAD_FEEDRATE));//do previous moves, so Ramming text is visible

        setPhase(PhasesLoadUnload::Ejecting_stoppable, 99);
        unload_filament(RammingType::unload);

        set(LoadPhases_t::_init);
        break;
    default:
        set(LoadPhases_t::_finish);
    }
}

void Pause::loop_load_purge(Response response) {
    const float purge_ln = settings.purge_length;

    // transitions
    switch (getLoadPhase()) {
    case LoadPhases_t::_init:
        set(LoadPhases_t::wait_temp);
        break;
    case LoadPhases_t::wait_temp:
        if (ensureSafeTemperatureNotifyProgress(30, 50)) {
            set(LoadPhases_t::purge);
        }
        break;
    case LoadPhases_t::error_temp:
        set(LoadPhases_t::_finish);
        break;

    case LoadPhases_t::purge:
        // Extrude filament to get into hotend
        setPhase(PhasesLoadUnload::Purging_stoppable, 70);
        do_e_move_notify_progress_hotextrude(purge_ln, ADVANCED_PAUSE_PURGE_FEEDRATE, 70, 99);
        setPhase(PhasesLoadUnload::IsColorPurge, 99);
        set(LoadPhases_t::ask_is_color_correct);
        break;
    case LoadPhases_t::ask_is_color_correct: {
        if (response == Response::Purge_more) {
            set(LoadPhases_t::purge);
        }
        if (response == Response::Continue) {
            set(LoadPhases_t::_finish);
        }
    } break;
    default:
        set(LoadPhases_t::_finish);
    }
}

void Pause::loop_load_not_blocking(Response response) {
    const float purge_ln = settings.purge_length;

    // transitions
    switch (getLoadPhase()) {
    case LoadPhases_t::_init:
        set(LoadPhases_t::load_in_gear);
        break;
    case LoadPhases_t::load_in_gear: //slow load
        setPhase(PhasesLoadUnload::Inserting_stoppable, 10);
        do_e_move_notify_progress_coldextrude(settings.slow_load_length, FILAMENT_CHANGE_SLOW_LOAD_FEEDRATE, 10, 30); // TODO method without param using actual phase
        Filaments::Set(Filaments::GetToBeLoaded());
        set(LoadPhases_t::wait_temp);
        break;
    case LoadPhases_t::wait_temp:
        if (ensureSafeTemperatureNotifyProgress(30, 50)) {
            set(LoadPhases_t::long_load);
        }
        break;
    case LoadPhases_t::error_temp:
        set(LoadPhases_t::_finish);
        break;
    case LoadPhases_t::long_load:
        planner.settings.retract_acceleration = FILAMENT_CHANGE_FAST_LOAD_ACCEL;
        setPhase(PhasesLoadUnload::Loading_stoppable, 50);
        do_e_move_notify_progress_hotextrude(settings.fast_load_length, FILAMENT_CHANGE_FAST_LOAD_FEEDRATE, 50, 70);
        set(LoadPhases_t::purge);
        break;
    case LoadPhases_t::purge:
        // Extrude filament to get into hotend
        setPhase(PhasesLoadUnload::Purging_stoppable, 70);
        do_e_move_notify_progress_hotextrude(purge_ln, ADVANCED_PAUSE_PURGE_FEEDRATE, 70, 99);
        set(LoadPhases_t::_finish);
        break;
    default:
        set(LoadPhases_t::_finish);
    }
}

void Pause::loop_load_mmu(Response response) {
    const float purge_ln = settings.purge_length;

    // transitions
    switch (getLoadPhase()) {
    case LoadPhases_t::_init:
        Filaments::Set(Filaments::GetToBeLoaded());
        set(LoadPhases_t::wait_temp);
        break;
    case LoadPhases_t::load_in_gear: //only if user pressed retry
        setPhase(PhasesLoadUnload::Inserting_stoppable, 10);
        do_e_move_notify_progress_coldextrude(settings.slow_load_length, FILAMENT_CHANGE_SLOW_LOAD_FEEDRATE, 10, 30); // TODO method without param using actual phase
        set(LoadPhases_t::wait_temp);
        break;
    case LoadPhases_t::wait_temp:
        if (ensureSafeTemperatureNotifyProgress(30, 50)) {
            set(LoadPhases_t::long_load);
        }
        break;
    case LoadPhases_t::error_temp:
        set(LoadPhases_t::_finish);
        break;
    case LoadPhases_t::long_load:
        planner.settings.retract_acceleration = FILAMENT_CHANGE_FAST_LOAD_ACCEL;
        setPhase(PhasesLoadUnload::Loading_stoppable, 50);
        do_e_move_notify_progress_hotextrude(settings.fast_load_length, FILAMENT_CHANGE_FAST_LOAD_FEEDRATE, 50, 70);
        set(LoadPhases_t::purge);
        break;
    case LoadPhases_t::purge:
        // Extrude filament to get into hotend
        setPhase(PhasesLoadUnload::Purging_stoppable, 70);
        do_e_move_notify_progress_hotextrude(purge_ln, ADVANCED_PAUSE_PURGE_FEEDRATE, 70, 99);
        setPhase(PhasesLoadUnload::IsColor, 99);
        set(LoadPhases_t::ask_is_color_correct);
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
    case LoadPhases_t::eject: {

        setPhase(PhasesLoadUnload::Ramming_stoppable, 98);
        ram_filament(RammingType::unload);

        planner.synchronize(); // do_pause_e_move(0, (FILAMENT_CHANGE_UNLOAD_FEEDRATE));//do previous moves, so Ramming text is visible

        setPhase(PhasesLoadUnload::Ejecting_stoppable, 99);
        unload_filament(RammingType::unload);

        set(LoadPhases_t::load_in_gear);
    } break;
    default:
        set(LoadPhases_t::_finish);
    }
}

void Pause::loop_autoload(Response response) {
    const float purge_ln = settings.purge_length;

    // transitions
    switch (getLoadPhase()) {
    case LoadPhases_t::_init:
        // we have already loaded the filament in gear, now just wait for temperature to rise
        Filaments::Set(Filaments::GetToBeLoaded());
        set(LoadPhases_t::wait_temp);
        break;
    case LoadPhases_t::check_filament_sensor_and_user_push__ask:
        if (FSensors_instance().HasNotFilament()) {
            setPhase(PhasesLoadUnload::MakeSureInserted_stoppable);
        } else {
            setPhase(PhasesLoadUnload::UserPush_stoppable);
            if (response == Response::Continue) {
                set(LoadPhases_t::load_in_gear);
            }
        }
        if (response == Response::Stop)
            settings.do_stop = true;
        break;
    case LoadPhases_t::load_in_gear: //slow load
        setPhase(PhasesLoadUnload::Inserting_stoppable, 10);
        do_e_move_notify_progress_coldextrude(settings.slow_load_length, FILAMENT_CHANGE_SLOW_LOAD_FEEDRATE, 10, 30); // TODO method without param using actual phase
        Filaments::Set(Filaments::GetToBeLoaded());
        set(LoadPhases_t::wait_temp);
        break;
    case LoadPhases_t::wait_temp:
        if (ensureSafeTemperatureNotifyProgress(30, 50)) {
            set(LoadPhases_t::long_load);
        }
        break;
    case LoadPhases_t::error_temp:
        set(LoadPhases_t::_finish);
        break;
    case LoadPhases_t::long_load:
        planner.settings.retract_acceleration = FILAMENT_CHANGE_FAST_LOAD_ACCEL;
        setPhase(PhasesLoadUnload::Loading_stoppable, 50);
        do_e_move_notify_progress_hotextrude(settings.fast_load_length, FILAMENT_CHANGE_FAST_LOAD_FEEDRATE, 50, 70);
        set(LoadPhases_t::purge);
        break;
    case LoadPhases_t::purge:
        // Extrude filament to get into hotend
        setPhase(PhasesLoadUnload::Purging_stoppable, 70);
        do_e_move_notify_progress_hotextrude(purge_ln, ADVANCED_PAUSE_PURGE_FEEDRATE, 70, 99);
        setPhase(PhasesLoadUnload::IsColor, 99);
        set(LoadPhases_t::ask_is_color_correct);
        break;
    case LoadPhases_t::ask_is_color_correct:
        if (response == Response::Purge_more) {
            set(LoadPhases_t::purge);
        }
        if (response == Response::Retry) {
            set(LoadPhases_t::eject);
        }
        if (response == Response::Continue) {
            set(LoadPhases_t::_finish);
        }
        break;
    case LoadPhases_t::eject:
        setPhase(PhasesLoadUnload::Ramming_stoppable, 98);
        ram_filament(RammingType::unload);

        planner.synchronize(); // do_pause_e_move(0, (FILAMENT_CHANGE_UNLOAD_FEEDRATE));//do previous moves, so Ramming text is visible

        setPhase(PhasesLoadUnload::Ejecting_stoppable, 99);
        unload_filament(RammingType::unload);

        set(LoadPhases_t::check_filament_sensor_and_user_push__ask);
        break;
    default:
        set(LoadPhases_t::_finish);
    }
}

void Pause::loop_loadToGear(Response response) {
    // transitions
    switch (getLoadPhase()) {
    case LoadPhases_t::_init:
        set(LoadPhases_t::load_in_gear);
        break;
    case LoadPhases_t::load_in_gear:
        setPhase(PhasesLoadUnload::Inserting_stoppable, 10);
        do_e_move_notify_progress_coldextrude(settings.slow_load_length, FILAMENT_CHANGE_SLOW_LOAD_FEEDRATE, 10, 30); // TODO method without param using actual phase
        set(LoadPhases_t::_finish);
        break;
    default:
        set(LoadPhases_t::_finish);
    }
}

void Pause::loop_load_change(Response response) {
    const float purge_ln = settings.purge_length;

    // transitions
    switch (getLoadPhase()) {
    case LoadPhases_t::_init:
        set(LoadPhases_t::check_filament_sensor_and_user_push__ask);
        break;
    case LoadPhases_t::check_filament_sensor_and_user_push__ask:
        if (FSensors_instance().HasNotFilament()) {
            setPhase(PhasesLoadUnload::MakeSureInserted_unstoppable);
        } else {
            setPhase(PhasesLoadUnload::UserPush_stoppable);
            if (response == Response::Continue) {
                set(LoadPhases_t::load_in_gear);
            }
        }
        if (response == Response::Stop)
            settings.do_stop = true;
        break;
    case LoadPhases_t::load_in_gear: //slow load
        setPhase(PhasesLoadUnload::Inserting_unstoppable, 10);
        do_e_move_notify_progress_coldextrude(settings.slow_load_length, FILAMENT_CHANGE_SLOW_LOAD_FEEDRATE, 10, 30); // TODO method without param using actual phase
        Filaments::Set(Filaments::GetToBeLoaded());
        set(LoadPhases_t::wait_temp);
        break;
    case LoadPhases_t::wait_temp:
        if (ensureSafeTemperatureNotifyProgress(30, 50)) {
            set(LoadPhases_t::long_load);
        }
        break;
    case LoadPhases_t::error_temp:
        set(LoadPhases_t::_finish);
        break;
    case LoadPhases_t::long_load:
        planner.settings.retract_acceleration = FILAMENT_CHANGE_FAST_LOAD_ACCEL;
        setPhase(PhasesLoadUnload::Loading_unstoppable, 50);
        do_e_move_notify_progress_hotextrude(settings.fast_load_length, FILAMENT_CHANGE_FAST_LOAD_FEEDRATE, 50, 70);
        set(LoadPhases_t::purge);
        break;
    case LoadPhases_t::purge:
        // Extrude filament to get into hotend
        setPhase(PhasesLoadUnload::Purging_unstoppable, 70);
        do_e_move_notify_progress_hotextrude(purge_ln, ADVANCED_PAUSE_PURGE_FEEDRATE, 70, 99);
        setPhase(PhasesLoadUnload::IsColor, 99);
        set(LoadPhases_t::ask_is_color_correct);
        break;
    case LoadPhases_t::ask_is_color_correct:
        if (response == Response::Purge_more) {
            set(LoadPhases_t::purge);
        }
        if (response == Response::Retry) {
            set(LoadPhases_t::eject);
        }
        if (response == Response::Continue) {
            set(LoadPhases_t::_finish);
        }
        break;
    case LoadPhases_t::eject:
        setPhase(PhasesLoadUnload::Ramming_unstoppable, 98);
        ram_filament(RammingType::unload);

        planner.synchronize(); // do_pause_e_move(0, (FILAMENT_CHANGE_UNLOAD_FEEDRATE));//do previous moves, so Ramming text is visible

        setPhase(PhasesLoadUnload::Ejecting_unstoppable, 99);
        unload_filament(RammingType::unload);

        set(LoadPhases_t::_init);
        break;
    default:
        set(LoadPhases_t::_finish);
    }
}

bool Pause::UnloadFromGear() {
    FSM_HolderLoadUnload H(*this, LoadUnloadMode::Unload);
    return filamentUnload(&Pause::loop_unloadFromGear);
}

bool Pause::FilamentUnload(const pause::Settings &settings_) {
    settings = settings_;
    FSM_HolderLoadUnload H(*this, LoadUnloadMode::Unload);
    return filamentUnload(FSensors_instance().HasMMU() ? &Pause::loop_unload_mmu : &Pause::loop_unload);
}

bool Pause::FilamentUnload_AskUnloaded(const pause::Settings &settings_) {
    settings = settings_;
    FSM_HolderLoadUnload H(*this, LoadUnloadMode::Unload);
    return filamentUnload(&Pause::loop_unload_AskUnloaded);
    // TODO specifi behavior for FSensors_instance().HasMMU()
}

bool Pause::FilamentLoad(const pause::Settings &settings_) {
    settings = settings_;
    FSM_HolderLoadUnload H(*this, settings.fast_load_length ? LoadUnloadMode::Load : LoadUnloadMode::Purge);
    return filamentLoad(FSensors_instance().HasMMU() ? &Pause::loop_load_mmu : (settings.fast_load_length ? &Pause::loop_load : &Pause::loop_load_purge));
}

bool Pause::FilamentLoadNotBlocking(const pause::Settings &settings_) {
    settings = settings_;
    FSM_HolderLoadUnload H(*this, LoadUnloadMode::Load);
    return filamentLoad(&Pause::loop_load_not_blocking);
}

bool Pause::FilamentAutoload(const pause::Settings &settings_) {
    settings = settings_;
    FSM_HolderLoadUnload H(*this, LoadUnloadMode::Load);
    return filamentLoad(&Pause::loop_autoload);
}

bool Pause::LoadToGear(const pause::Settings &settings_) {
    settings = settings_;
    FSM_HolderLoadUnload H(*this, LoadUnloadMode::Load);
    return filamentLoad(&Pause::loop_loadToGear);
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
bool Pause::filamentUnload(loop_fn fn) {
    if (process_stop()) // What is this doing here?
        return false;

    if (!ensureSafeTemperatureNotifyProgress(0, 50) && fn != &Pause::loop_unloadFromGear)
        return false;

    set(UnloadPhases_t::_init);

    return invoke_loop(fn);
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
bool Pause::filamentLoad(loop_fn fn) {

    // actual temperature does not matter, only target
    if (!is_target_temperature_safe() && fn != &Pause::loop_loadToGear)
        return false;

    AutoRestore<float> AR(planner.settings.retract_acceleration);
    set(LoadPhases_t::_init);

    return invoke_loop(fn);
}

bool Pause::invoke_loop(loop_fn fn) {
#if ENABLED(PID_EXTRUSION_SCALING)
    bool extrusionScalingEnabled = thermalManager.getExtrusionScalingEnabled();
    thermalManager.setExtrusionScalingEnabled(false);
#endif //ENABLED(PID_EXTRUSION_SCALING)

    bool ret = true;
    while (!finished()) {
        ret = !process_stop();
        if (ret) {
            const Response response = getResponse();
            std::invoke(fn, *this, response);
        } else {
            set(intFinishVal);
            continue;
        }
        ret = !process_stop(); //why is this 2nd call here ???, some workaround ???
        if (ret) {
            idle(true, true); // idle loop to prevent wdt and manage heaters etc, true == do not shutdown steppers
        } else {
            set(intFinishVal);
            continue;
        }
    };

#if ENABLED(PID_EXTRUSION_SCALING)
    thermalManager.setExtrusionScalingEnabled(extrusionScalingEnabled);
#endif //ENABLED(PID_EXTRUSION_SCALING)

    return ret;
}

void Pause::loop_unload(Response response) {
    // transitions
    switch (getUnloadPhase()) {
    case UnloadPhases_t::_init:
        set(UnloadPhases_t::ram_sequence);
        break;
    case UnloadPhases_t::ram_sequence:
        setPhase(PhasesLoadUnload::Ramming_stoppable, 50);
        ram_filament(RammingType::unload);
        set(UnloadPhases_t::unload);
        break;
    case UnloadPhases_t::unload:
        setPhase(PhasesLoadUnload::Unloading_stoppable, 51);
        unload_filament(RammingType::unload);
        if (settings.do_stop)
            break;

        Filaments::Set(filament_t::NONE);
        set(UnloadPhases_t::_finish);
        break;
    default:
        set(UnloadPhases_t::_finish);
    }
}

void Pause::loop_unload_AskUnloaded(Response response) {
    // transitions
    switch (getUnloadPhase()) {
    case UnloadPhases_t::_init:
        set(UnloadPhases_t::ram_sequence);
        break;
    case UnloadPhases_t::ram_sequence:
        setPhase(PhasesLoadUnload::Ramming_stoppable, 50);
        ram_filament(RammingType::unload);
        set(UnloadPhases_t::unload);
        break;
    case UnloadPhases_t::unload:
        setPhase(PhasesLoadUnload::Unloading_stoppable, 51);
        unload_filament(RammingType::unload);
        if (settings.do_stop)
            break;

        Filaments::Set(filament_t::NONE);
        setPhase(PhasesLoadUnload::IsFilamentUnloaded, 100);
        set(UnloadPhases_t::unloaded__ask);
        break;
    case UnloadPhases_t::unloaded__ask:
        if (response == Response::Yes) {
            set(UnloadPhases_t::filament_not_in_fs);
            break;
        }
        if (response == Response::No) {
            setPhase(PhasesLoadUnload::ManualUnload, 100);
            disable_e_stepper(active_extruder);
            set(UnloadPhases_t::manual_unload);
        }
        break;
    case UnloadPhases_t::filament_not_in_fs:
        setPhase(PhasesLoadUnload::FilamentNotInFS);
        if (!FSensors_instance().PrinterHasFilament()) {
            set(UnloadPhases_t::_finish);
        }
        break;
    case UnloadPhases_t::manual_unload:
        if (response == Response::Continue) {
            enable_e_steppers();
            set(UnloadPhases_t::filament_not_in_fs);
        }
        break;

    default:
        set(UnloadPhases_t::_finish);
    }
}

void Pause::loop_unload_mmu(Response response) {
    // transitions
    switch (getUnloadPhase()) {
    case UnloadPhases_t::_init:
        set(UnloadPhases_t::ram_sequence);
        break;
    case UnloadPhases_t::ram_sequence:
        setPhase(PhasesLoadUnload::Ramming_stoppable, 50);
        ram_filament(RammingType::unload);
        set(UnloadPhases_t::unload);
        break;
    case UnloadPhases_t::unload:
        setPhase(PhasesLoadUnload::Unloading_stoppable, 51);
        unload_filament(RammingType::unload);
        if (settings.do_stop)
            break;

        Filaments::Set(filament_t::NONE);
        set(UnloadPhases_t::run_mmu_unload);
        break;
    case UnloadPhases_t::run_mmu_unload:
        set(UnloadPhases_t::_finish);
        break;
    default:
        set(UnloadPhases_t::_finish);
    }
}

void Pause::loop_unloadFromGear(Response response) {
    // transitions
    switch (getUnloadPhase()) {
    case UnloadPhases_t::_init:
        set(UnloadPhases_t::unload_from_gear);
        break;
    case UnloadPhases_t::unload_from_gear:
        setPhase(PhasesLoadUnload::Unloading_stoppable, 0);
        do_e_move_notify_progress_coldextrude(-settings.slow_load_length * (float)1.5, FILAMENT_CHANGE_FAST_LOAD_FEEDRATE, 0, 100);
        set(UnloadPhases_t::_finish);
        break;
    default:
        set(UnloadPhases_t::_finish);
    }
}

void Pause::loop_unload_change(Response response) {
    // transitions
    switch (getUnloadPhase()) {
    case UnloadPhases_t::_init:
        set(UnloadPhases_t::ram_sequence);
        break;
    case UnloadPhases_t::ram_sequence:
        setPhase(PhasesLoadUnload::Ramming_unstoppable, 50);
        ram_filament(RammingType::runout);
        set(UnloadPhases_t::unload);
        break;
    case UnloadPhases_t::unload:
        setPhase(PhasesLoadUnload::Unloading_unstoppable, 51);
        unload_filament(RammingType::runout);
        if (settings.do_stop)
            break;

        Filaments::Set(filament_t::NONE);

        setPhase(PhasesLoadUnload::IsFilamentUnloaded, 100);
        set(UnloadPhases_t::unloaded__ask);
        break;
    case UnloadPhases_t::unloaded__ask:
        if (response == Response::Yes) {
            set(UnloadPhases_t::filament_not_in_fs);
            break;
        }
        if (response == Response::No) {
            setPhase(PhasesLoadUnload::ManualUnload, 100);
            disable_e_stepper(active_extruder);
            set(UnloadPhases_t::manual_unload);
        }
        break;
    case UnloadPhases_t::filament_not_in_fs:
        setPhase(PhasesLoadUnload::FilamentNotInFS);
        if (!FSensors_instance().PrinterHasFilament()) {
            set(UnloadPhases_t::_finish);
        }
        break;
    case UnloadPhases_t::manual_unload:
        if (response == Response::Continue) {
            enable_e_steppers();
            set(UnloadPhases_t::filament_not_in_fs);
        }
        break;
    default:
        set(UnloadPhases_t::_finish);
    }
}

/*****************************************************************************/
//park moves
uint32_t Pause::parkMoveZPercent(float z_move_len, float xy_move_len) const {
    const float Z_time_ratio = std::abs(z_move_len / float(NOZZLE_PARK_Z_FEEDRATE));
    const float XY_time_ratio = std::abs(xy_move_len / float(NOZZLE_PARK_XY_FEEDRATE));

    if (!isfinite(Z_time_ratio))
        return 100;
    if (!isfinite(XY_time_ratio))
        return 0;
    if ((Z_time_ratio + XY_time_ratio) == 0)
        return 50; // due abs should not happen except both == 0

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

    if (pos_nan.y)
        return true;
    if (pos_nan.x)
        return false;

    return std::abs(pos0.x - pos1.x) > std::abs(pos0.y - pos1.y);
}

bool Pause::wait_or_stop() {
    while (planner.busy()) {
        if (check_user_stop())
            return true;
        idle(true, true);
    }
    return false;
}

void Pause::park_nozzle_and_notify() {
    setPhase(settings.can_stop ? PhasesLoadUnload::Parking_stoppable : PhasesLoadUnload::Parking_unstoppable);
    // Initial retract before move to filament change position
    if (settings.retract && thermalManager.hotEnoughToExtrude(active_extruder))
        do_pause_e_move(settings.retract, PAUSE_PARK_RETRACT_FEEDRATE);

    const float target_Z = settings.park_pos.z;
    const float Z_len = current_position.z - target_Z; // sign does not matter

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

    // move by z_lift, scope for Notifier_POS_Z
    if (isfinite(target_Z)) {
        Notifier_POS_Z N(ClientFSM::Load_unload, getPhaseIndex(), current_position.z, target_Z, 0, parkMoveZPercent(Z_len, XY_len));
        plan_park_move_to(current_position.x, current_position.y, target_Z, NOZZLE_PARK_XY_FEEDRATE, NOZZLE_PARK_Z_FEEDRATE);
        if (wait_or_stop())
            return;
    }

    // move to (x_pos, y_pos)
    if (XY_len != 0) {
        //home the X or Y axis if it is not homed and we want to move it
        //homing is after Z move to be clear of all obstacles
        //Should not affect other operations than Load/Unload/Change filament run from home screen without homing. We are homed during print
        LOOP_XY(axis) {
            // TODO: make homeaxis non-blocking to allow quick_stop
            if (!isnan(settings.park_pos.pos[axis]) && axes_need_homing(_BV(axis)))
                GcodeSuite::G28_no_parser(false, false, 0, false, axis == X_AXIS, axis == Y_AXIS, false);
            if (check_user_stop())
                return;
            if (isnan(settings.park_pos.pos[axis]))
                settings.park_pos.pos[axis] = current_position.pos[axis];
        }

        if (x_greater_than_y) {
            Notifier_POS_X N(ClientFSM::Load_unload, getPhaseIndex(), begin_pos, end_pos, parkMoveZPercent(Z_len, XY_len), 100); //from Z% to 100%
            plan_park_move_to_xyz(settings.park_pos, NOZZLE_PARK_XY_FEEDRATE, NOZZLE_PARK_Z_FEEDRATE);
            if (wait_or_stop())
                return;
        } else {
            Notifier_POS_Y N(ClientFSM::Load_unload, getPhaseIndex(), begin_pos, end_pos, parkMoveZPercent(Z_len, XY_len), 100); //from Z% to 100%
            plan_park_move_to_xyz(settings.park_pos, NOZZLE_PARK_XY_FEEDRATE, NOZZLE_PARK_Z_FEEDRATE);
            if (wait_or_stop())
                return;
        }
    }

    report_current_position();
}

void Pause::unpark_nozzle_and_notify() {
    if (settings.resume_pos.x == NAN || settings.resume_pos.y == NAN || settings.resume_pos.z == NAN)
        return;

    setPhase(PhasesLoadUnload::Unparking);
    // Move XY to starting position, then Z
    const bool x_greater_than_y = parkMoveXGreaterThanY(current_position, settings.resume_pos);
    const float &begin_pos = x_greater_than_y ? current_position.x : current_position.y;
    const float &end_pos = x_greater_than_y ? settings.resume_pos.x : settings.resume_pos.y;

    const float Z_len = current_position.z - settings.resume_pos.z; // sign does not matter, does not check Z max val (unlike park_nozzle_and_notify)
    const float XY_len = begin_pos - end_pos;                       // sign does not matter

    // home the axis if it is not homed
    // we can move only one axis during parking and not home the other one and then unpark and move the not homed one, so we need to home it
    LOOP_XY(axis) {
        if (!isnan(settings.park_pos.pos[axis]) && axes_need_homing(_BV(axis)))
            GcodeSuite::G28_no_parser(false, false, 0, false, axis == X_AXIS, axis == Y_AXIS, false);
    }

    if (x_greater_than_y) {
        Notifier_POS_X N(ClientFSM::Load_unload, getPhaseIndex(), begin_pos, end_pos, 0, parkMoveXYPercent(Z_len, XY_len));
        do_blocking_move_to_xy(settings.resume_pos, NOZZLE_UNPARK_XY_FEEDRATE);
    } else {
        Notifier_POS_Y N(ClientFSM::Load_unload, getPhaseIndex(), begin_pos, end_pos, 0, parkMoveXYPercent(Z_len, XY_len));
        do_blocking_move_to_xy(settings.resume_pos, NOZZLE_UNPARK_XY_FEEDRATE);
    }

    // Move Z_AXIS to saved position, scope for Notifier_POS_Z
    {
        Notifier_POS_Z N(ClientFSM::Load_unload, getPhaseIndex(), current_position.z, settings.resume_pos.z, parkMoveXYPercent(Z_len, XY_len), 100); //from XY% to 100%
        do_blocking_move_to_z(settings.resume_pos.z, feedRate_t(NOZZLE_PARK_Z_FEEDRATE));
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
void Pause::FilamentChange(const pause::Settings &settings_) {
    settings = settings_;
    settings.can_stop = false;
    if (did_pause_print)
        return; // already paused

    if (!DEBUGGING(DRYRUN) && settings.unload_length && thermalManager.targetTooColdToExtrude(active_extruder)) {
        SERIAL_ECHO_MSG(MSG_ERR_HOTEND_TOO_COLD);
        return; // unable to reach safe temperature
    }

    // Indicate that the printer is paused
    ++did_pause_print;

    print_job_timer.pause();

    // Wait for buffered blocks to complete
    planner.synchronize();
    //Lock filament sensor, so it does not enqueue new M600, beacuse of filament run out
    FS_EventAutolock runout_disable;

#if ENABLED(ADVANCED_PAUSE_FANS_PAUSE) && FAN_COUNT > 0
    thermalManager.set_fans_paused(true);
#endif

    {
        FSM_HolderLoadUnload H(*this, LoadUnloadMode::Change);

        if (settings.unload_length) // Unload the filament
            filamentUnload(&Pause::loop_unload_change);
        // Feed a little bit of filament to stabilize pressure in nozzle
        if (filamentLoad(&Pause::loop_load_change)) {
            plan_e_move(5, 10);
            planner.synchronize();
            delay(500);
        }
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
    planner.set_e_position_mm((destination.e = current_position.e = settings.resume_pos.e));

    --did_pause_print;

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
void Pause::ram_filament(const Pause::RammingType type) {
    const RammingSequence &sequence = get_ramming_sequence(type);

    constexpr float mm_per_minute = 1 / 60.f;
    // ram filament
    for (auto &elem : sequence) {
        plan_e_move(elem.e, elem.feedrate * mm_per_minute);
        if (check_user_stop())
            return;
    }
}

void Pause::unload_filament(const Pause::RammingType type) {
    const RammingSequence &sequence = get_ramming_sequence(type);

    const float saved_acceleration = planner.settings.retract_acceleration;
    planner.settings.retract_acceleration = FILAMENT_CHANGE_UNLOAD_ACCEL;

    // subtract the already performed extruder movement from the total unload length and ensure it is negative
    float remaining_unload_length = -(std::abs(settings.unload_length) - sequence.unload_length);
    if (remaining_unload_length > .0f) {
        remaining_unload_length = .0f;
    }
    do_e_move_notify_progress_hotextrude(remaining_unload_length, (FILAMENT_CHANGE_UNLOAD_FEEDRATE), 51, 99);

    planner.settings.retract_acceleration = saved_acceleration;
}
const RammingSequence &Pause::get_ramming_sequence(const RammingType type) const {
    switch (type) {
    case Pause::RammingType::runout:
        return runoutRammingSequence;
    case Pause::RammingType::unload:
        return unloadRammingSequence;
    }
    return unloadRammingSequence;
}

bool Pause::check_user_stop() {
    const Response response = getResponse();
    if (response != Response::Stop)
        return false;

    settings.do_stop = true;
    planner.quick_stop();
    while (planner.has_blocks_queued())
        loop();
    planner.resume_queuing();
    set_all_unhomed();
    set_all_unknown();
    xyze_pos_t real_current_position;
    real_current_position[E_AXIS] = 0;
    LOOP_XYZ(i) {
        real_current_position[i] = planner.get_axis_position_mm((AxisEnum)i);
    }
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
    return true;
}

/*****************************************************************************/
//Pause::FSM_HolderLoadUnload

void Pause::FSM_HolderLoadUnload::bindToSafetyTimer() {
    SafetyTimer::Instance().BindPause(pause);
}

void Pause::FSM_HolderLoadUnload::unbindFromSafetyTimer() {
    SafetyTimer::Instance().UnbindPause(pause);
}

Pause::FSM_HolderLoadUnload::FSM_HolderLoadUnload(Pause &p, LoadUnloadMode mode)
    : FSM_Holder(ClientFSM::Load_unload, uint8_t(mode))
    , pause(p) {
    pause.clrRestoreTemp();
    bindToSafetyTimer();
    pause.park_nozzle_and_notify();
}

Pause::FSM_HolderLoadUnload::~FSM_HolderLoadUnload() {
    pause.RestoreTemp();

    const float min_layer_h = 0.05f;
    //do not unpark and wait for temp if not homed or z park len is 0
    if (!axes_need_homing() && pause.settings.resume_pos.z != NAN && std::abs(current_position.z - pause.settings.resume_pos.z) >= min_layer_h) {
        if (!pause.ensureSafeTemperatureNotifyProgress(0, 100))
            return;
        pause.unpark_nozzle_and_notify();
    }
    unbindFromSafetyTimer(); //unbind must be last action, without it Pause cannot block safety timer
}
