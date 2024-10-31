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
#include <option/has_side_fsensor.h>

// @brief With Z unhomed, ensure that it is at least amount_mm above bed.
void unhomed_z_lift(float amount_mm);

class PausePrivatePhase : public IPause {
    PhasesLoadUnload phase; // needed for CanSafetyTimerExpire
    int load_unload_shared_phase; // shared variable for UnloadPhases_t and LoadPhases_t
    std::optional<LoadUnloadMode> load_unload_mode = std::nullopt;

    float nozzle_restore_temp[HOTENDS];
    float bed_restore_temp;

public:
    static constexpr int intFinishVal = INT_MAX;

protected:
    enum class UnloadPhases_t {
        _finish = intFinishVal,
        _init = 0,
        filament_stuck_wait_user,
        ram_sequence,
        unload,
        unloaded__ask,
        manual_unload,
        filament_not_in_fs,
        unload_from_gear,
        run_mmu_eject,
        _last = run_mmu_eject,

    };

    enum class LoadPhases_t {
        _finish = intFinishVal,
        _init = int(UnloadPhases_t::_last) + 1,
        check_filament_sensor_and_user_push__ask, // must be one phase because of button click
#if HAS_SIDE_FSENSOR()
        await_filament,
        assist_filament_insertion,
#endif
        load_in_gear,
        move_to_purge,
        wait_temp,
        error_temp,
        long_load,
        purge,
        ask_is_color_correct,
        eject,
        unloaded_ask,
        manual_unload,
        ask_mmu_load_filament,
        mmu_load_filament,
        _last = mmu_load_filament,
    };

    PausePrivatePhase();
    void setPhase(PhasesLoadUnload ph, uint8_t progress = 0);
    PhasesLoadUnload getPhase() const;

    // auto restores temp turned off by safety timer,
    // it is also restored by SafetyTimer on any user click
    // cannot guarante that SafetyTimer will happen first, so have to do it on both places
    Response getResponse();

    // use UnloadPhases_t or LoadPhases_t
    template <class ENUM>
    ENUM get() {
        if (load_unload_shared_phase < int(ENUM::_init)) {
            return ENUM::_finish;
        }
        if (load_unload_shared_phase > int(ENUM::_last)) {
            return ENUM::_finish;
        }
        return ENUM(load_unload_shared_phase);
    }

    LoadPhases_t getLoadPhase() {
        return get<LoadPhases_t>();
    }
    UnloadPhases_t getUnloadPhase() {
        return get<UnloadPhases_t>();
    }

    template <class ENUM>
    void set(ENUM en) {
        load_unload_shared_phase = int(en);
    }

    // use only when necessary
    bool finished() { return load_unload_shared_phase == intFinishVal; }

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

class RammingSequence;

// used by load / unlaod /change filament
class Pause : public PausePrivatePhase {
    pause::Settings settings;
    bool user_stop_pending = false;

    uint32_t start_time_ms { 0 };
#if !HAS_HUMAN_INTERACTIONS()
    uint32_t runout_timer_ms { 0 };
#endif

    // singleton
    Pause() = default;
    Pause(const Pause &) = delete;
    Pause &operator=(const Pause &) = delete;

    static constexpr const float heating_phase_min_hotend_diff = 5.0F;

public:
    static constexpr const float minimal_purge = 1;
    static Pause &Instance();

    bool FilamentUnload(const pause::Settings &settings_);
    bool FilamentUnload_AskUnloaded(const pause::Settings &settings_);
    bool FilamentAutoload(const pause::Settings &settings_);
    bool LoadToGear(const pause::Settings &settings_);

    /**
     * @brief Change tool before load/unload.
     * @param target_extruder change to this tool [indexed from 0]
     * @param mode before which operation
     * @param settings_ config for park and othe Pause stuff
     * @return true on success
     */
    bool ToolChange(uint8_t target_extruder, LoadUnloadMode mode, const pause::Settings &settings_);

    bool UnloadFromGear(); // does not need config
    bool FilamentLoad(const pause::Settings &settings_);
    bool FilamentLoadNotBlocking(const pause::Settings &settings_);
    void FilamentChange(const pause::Settings &settings_);

    void finalize_user_stop();

    template <class ENUM>
    void set_timed(ENUM en) {
        start_time_ms = ticks_ms();
        set(en);
    }

private:
    using loop_fn = void (Pause::*)(Response response);
    void loop_unload(Response response);
    void loop_unload_AskUnloaded(Response response);
    void loop_unload_mmu_change(Response response);
    void loop_unloadFromGear(Response response); // autoload abort
    void loop_unload_change(Response response);
    void loop_unload_filament_stuck(Response response);

    enum class CommonUnloadType : uint8_t {
        standard,
        ask_unloaded,
        unload_from_gears,
        filament_change,
        filament_stuck,
    };
    void loop_unload_common(Response response, CommonUnloadType unload_type);
    // TODO loop_unload_change_mmu

    void loop_load(Response response);
    void loop_load_purge(Response response);
    void loop_load_not_blocking(Response response); // no buttons at all - printer without GUI etc
    void loop_load_mmu(Response response);
    void loop_load_mmu_change(Response response);
    void loop_autoload(Response response); // todo force remove filament in retry
    void loop_load_to_gear(Response response);
    void loop_load_change(Response response);
    void loop_load_filament_stuck(Response response);

    enum class CommonLoadType : uint8_t {
        load_to_gear,
        standard,
        autoload,
        filament_change,
        filament_stuck,
        not_blocking,
        load_purge,
        mmu, ///< MMU load to nozzle
        mmu_change, ///< MMU filament change (for example filament runout)
    };
    void loop_load_common(Response response, CommonLoadType load_type);
    // TODO loop_load_change_mmu

    // does not create FSM_HolderLoadUnload
    bool invoke_loop(loop_fn fn); // shared load/unload code
    bool filamentUnload(loop_fn fn);
    bool filamentLoad(loop_fn fn);

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
    void handle_filament_removal(LoadPhases_t phase_to_set); //<checks if filament is present if not it sets different phase

    enum class RammingType {
        unload,
        runout
    };

    void ram_filament(const RammingType sequence);
    void unload_filament(const RammingType sequence);
    const RammingSequence &get_ramming_sequence(const RammingType type) const;

    // create finite state machine and automatically destroy it at the end of scope
    // parks in ctor and unparks in dtor
    class FSM_HolderLoadUnload : public marlin_server::FSM_Holder {
        Pause &pause;

        void bindToSafetyTimer();
        void unbindFromSafetyTimer();
        static bool active; // we currently support only 1 instance
    public:
        FSM_HolderLoadUnload(Pause &p, LoadUnloadMode mode, bool park);
        ~FSM_HolderLoadUnload();
        friend class Pause;
    };

public:
    static bool IsFsmActive() { return FSM_HolderLoadUnload::active; }
};
