/**
 * @file pause_stubbed.hpp
 * @author Radek Vana
 * @brief stubbed version of marlin pause.hpp
 * mainly used for load / unload / change filament
 * @date 2020-12-18
 */

#pragma once
#include <stdint.h>
#include <limits.h>
#include "pause_settings.hpp"
#include "client_response.hpp"
#include "pause_settings.hpp"
#include "marlin_server.hpp"
#include "IPause.hpp"
#include <array>
#include "Marlin/src/libs/stopwatch.h"

#include <option/has_human_interactions.h>
#include <option/has_nozzle_cleaner.h>
#include <option/has_side_fsensor.h>

// @brief With Z unhomed, ensure that it is at least amount_mm above bed.
void unhomed_z_lift(float amount_mm);

class PausePrivatePhase : public IPause {
protected:
    enum class LoadState {
        _finish = INT_MAX,
        start = 0,
        unload_start,
#if HAS_LOADCELL()
        filament_stuck_ask,
#endif
        ram_sequence,
        unload,
        unloaded_ask,
        manual_unload,
        filament_not_in_fs,
        unload_from_gears,
#if HAS_NOZZLE_CLEANER()
        unload_nozzle_clean,
#endif
        unload_finish_or_change,
        load_start,
        filament_push_ask, // must be one phase because of button click
        await_filament,
        assist_insertion,
        load_to_gears,
        move_to_purge,
        wait_temp,
        long_load,
        purge,
        color_correct_ask,
        eject,
#if HAS_MMU2()
        mmu_load_ask,
        mmu_load,
#endif
#if HAS_NOZZLE_CLEANER()
        load_nozzle_clean,
#endif
        load_prime,
        _last = load_prime,
    };

private:
    PhasesLoadUnload phase; // needed for CanSafetyTimerExpire
    std::optional<LoadUnloadMode> load_unload_mode = std::nullopt;

    float nozzle_restore_temp[HOTENDS];
    float bed_restore_temp;

protected:
    LoadState state { LoadState::unload_start };

    PausePrivatePhase();
    void setPhase(PhasesLoadUnload ph, uint8_t progress = 0);
    PhasesLoadUnload getPhase() const;

    // auto restores temp turned off by safety timer,
    // it is also restored by SafetyTimer on any user click
    // cannot guarante that SafetyTimer will happen first, so have to do it on both places
    Response getResponse();

    LoadState get_state() {
        return state;
    }

    void set(LoadState s) {
        state = s;
    }

    // use only when necessary
    bool finished() { return state == LoadState::_finish; }

    void clrRestoreTemp();

public:
    constexpr uint8_t getPhaseIndex() const {
        return GetPhaseIndex(phase);
    }

    virtual void RestoreTemp() override;
    virtual bool CanSafetyTimerExpire() const override; // evaluate if client can click == safety timer can expire
    virtual void NotifyExpiredFromSafetyTimer() override;
    virtual bool HasTempToRestore() const override;

    void set_mode(LoadUnloadMode mode) { load_unload_mode = mode; }
    void clr_mode() { load_unload_mode = std::nullopt; }
    std::optional<LoadUnloadMode> get_mode() const { return load_unload_mode; }
};

// used by load / unlaod /change filament
class Pause : public PausePrivatePhase {
    pause::Settings settings;
    bool user_stop_pending = false;

    uint32_t start_time_ms { 0 };
    uint32_t runout_timer_ms { 0 };

    /// How much filament was retracted thanks to ramming
    float ram_retracted_distance = 0;

    // singleton
    Pause() = default;
    Pause(const Pause &) = delete;
    Pause &operator=(const Pause &) = delete;

    static constexpr const float heating_phase_min_hotend_diff = 5.0F;

public:
    enum class LoadType : uint8_t {
        load,
        autoload,
        load_to_gears,
        load_purge,
        unload,
        unload_confirm,
        unload_from_gears,
        filament_change,
        filament_stuck,
    };

    static constexpr const float minimal_purge = 1;
    static Pause &Instance();

    bool perform(LoadType load_type, const pause::Settings &settings);

    /**
     * @brief Change tool before load/unload.
     * @param target_extruder change to this tool [indexed from 0]
     * @param mode before which operation
     * @param settings_ config for park and othe Pause stuff
     * @return true on success
     */
    bool tool_change(uint8_t target_extruder, LoadType load_type, const pause::Settings &settings_);

    void filament_change(const pause::Settings &settings_, bool is_filament_stuck);

    void finalize_user_stop();

    template <class ENUM>
    void set_timed(ENUM en) {
        start_time_ms = ticks_ms();
        set(en);
    }

private:
    LoadType load_type {};

    bool is_unstoppable() const;
    LoadUnloadMode get_load_unload_mode();
    bool should_park();

    void start_process(Response response);
    void unload_start_process(Response response);
#if HAS_LOADCELL()
    void filament_stuck_ask_process(Response response);
#endif
    void ram_sequence_process(Response response);
    void unload_process(Response response);
    void unloaded_ask_process(Response response);
    void manual_unload_process(Response response);
    void filament_not_in_fs_process(Response response);
    void unload_from_gears_process(Response response);
#if HAS_NOZZLE_CLEANER()
    void unload_nozzle_clean_process(Response response);
#endif
    void unload_finish_or_change_process(Response response);
    void load_start_process(Response response);
    void filament_push_ask_process(Response response);
    void await_filament_process(Response response);
    void assist_insertion_process(Response response);
    void load_to_gears_process(Response response);
    void move_to_purge_process(Response response);
    void wait_temp_process(Response response);
    void long_load_process(Response response);
    void purge_process(Response response);
    void color_correct_ask_process(Response response);
    void eject_process(Response response);
#if HAS_MMU2()
    void mmu_load_ask_process(Response response);
    void mmu_load_process(Response response);
#endif
#if HAS_NOZZLE_CLEANER()
    void load_nozzle_clean_process(Response response);
#endif
    void load_prime_process(Response response);

    using StateHandler = void (Pause::*)(Response response);
    static constexpr EnumArray<LoadState, StateHandler, static_cast<int>(LoadState::_last) + 1> state_handlers {
        { LoadState::start, &Pause::start_process },
            { LoadState::unload_start, &Pause::unload_start_process },
#if HAS_LOADCELL()
            { LoadState::filament_stuck_ask, &Pause::filament_stuck_ask_process },
#endif
            { LoadState::ram_sequence, &Pause::ram_sequence_process },
            { LoadState::unload, &Pause::unload_process },
            { LoadState::unloaded_ask, &Pause::unloaded_ask_process },
            { LoadState::manual_unload, &Pause::manual_unload_process },
            { LoadState::filament_not_in_fs, &Pause::filament_not_in_fs_process },
            { LoadState::unload_from_gears, &Pause::unload_from_gears_process },
#if HAS_NOZZLE_CLEANER()
            { LoadState::unload_nozzle_clean, &Pause::unload_nozzle_clean_process },
#endif
            { LoadState::unload_finish_or_change, &Pause::unload_finish_or_change_process },
            { LoadState::load_start, &Pause::load_start_process },
            { LoadState::filament_push_ask, &Pause::filament_push_ask_process },
            { LoadState::await_filament, &Pause::await_filament_process },
            { LoadState::assist_insertion, &Pause::assist_insertion_process },
            { LoadState::load_to_gears, &Pause::load_to_gears_process },
            { LoadState::move_to_purge, &Pause::move_to_purge_process },
            { LoadState::wait_temp, &Pause::wait_temp_process },
            { LoadState::long_load, &Pause::long_load_process },
            { LoadState::purge, &Pause::purge_process },
            { LoadState::color_correct_ask, &Pause::color_correct_ask_process },
            { LoadState::eject, &Pause::eject_process },
#if HAS_MMU2()
            { LoadState::mmu_load_ask, &Pause::mmu_load_ask_process },
            { LoadState::mmu_load, &Pause::mmu_load_process },
#endif
#if HAS_NOZZLE_CLEANER()
            { LoadState::load_nozzle_clean, &Pause::load_nozzle_clean_process },
#endif
            { LoadState::load_prime, &Pause::load_prime_process },
    };

    // does not create FSM_HolderLoadUnload
    bool invoke_loop(); // shared load/unload code

    // park moves calculations
    uint32_t parkMoveZPercent(float z_move_len, float xy_move_len) const;
    uint32_t parkMoveXYPercent(float z_move_len, float xy_move_len) const;
    bool parkMoveXGreaterThanY(const xyz_pos_t &pos0, const xyz_pos_t &pos1) const;

    void unpark_nozzle_and_notify();
    void park_nozzle_and_notify();
    bool is_target_temperature_safe();

    /// Extrudes \p length .
    void plan_e_move(const float &length, const feedRate_t &fr_mm_s);

    bool ensureSafeTemperatureNotifyProgress(uint8_t progress_min, uint8_t progress_max);

    /// Moves the extruder by \p length . Notifies the FSM about progress.
    void do_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max);

    /// Moves the extruder by \p length . Does not mind the hotend being cold. Notifies the FSM about progress.
    void do_e_move_notify_progress_coldextrude(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max);

    /// Moves the extruder by \p length . Heats up for the move if necessary. Notifies the FSM about progress.
    void do_e_move_notify_progress_hotextrude(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max);

    bool check_user_stop(); //< stops motion and fsm and returns true it user triggered stop
    bool wait_for_motion_finish_or_user_stop(); //< waits until motion is finished; if stop is triggered then returns true
    bool process_stop();
    void handle_filament_removal(LoadState state_to_set); //<checks if filament is present if not it sets a different state

    /// To be called from states that are waiting for some filament sensor input (recovery strategy when FS has problems)
    /// If Help response is displayed, shows a help dialog and provides options to resolve
    void handle_help(Response response);

    void ram_filament(uint8_t progress_percent);
    void unload_filament();

    // create finite state machine and automatically destroy it at the end of scope
    // parks in ctor and unparks in dtor
    class FSM_HolderLoadUnload : public marlin_server::FSM_Holder {
        Pause &pause;

        void bindToSafetyTimer();
        void unbindFromSafetyTimer();
        static bool active; // we currently support only 1 instance
    public:
        FSM_HolderLoadUnload(Pause &p);
        ~FSM_HolderLoadUnload();
        friend class Pause;
    };

public:
    static bool IsFsmActive() { return FSM_HolderLoadUnload::active; }
};
