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

class PausePrivatePhase : public IPause {
    PhasesLoadUnload phase;       //needed for CanSafetyTimerExpire
    int load_unload_shared_phase; //shared variable for UnloadPhases_t and LoadPhases_t

    float nozzle_restore_temp;
    float bed_restore_temp;

public:
    static constexpr int intFinishVal = INT_MAX;

protected:
    enum class UnloadPhases_t {
        _finish = intFinishVal,
        _init = 0,
        ram_sequence,
        unload,
        unloaded__ask,
        manual_unload,
        filament_not_in_fs,
        unload_from_gear,
        run_mmu_unload,
        _last = run_mmu_unload,

    };

    enum class LoadPhases_t {
        _finish = intFinishVal,
        _init = int(UnloadPhases_t::_last) + 1,
        check_filament_sensor_and_user_push__ask, //must be one phase because of button click
        load_in_gear,
        wait_temp,
        error_temp,
        long_load,
        purge,
        ask_is_color_correct,
        eject,
        _last = eject,
    };

    PausePrivatePhase();
    void setPhase(PhasesLoadUnload ph, uint8_t progress = 0);
    PhasesLoadUnload getPhase() const;

    // auto restores temp turned off by safety timer,
    // it is also restored by SafetyTimer on any user click
    // cannot guarante that SafetyTimer will happen first, so have to do it on both places
    Response getResponse();

    constexpr uint8_t getPhaseIndex() const {
        return GetPhaseIndex(phase);
    }

    //use UnloadPhases_t or LoadPhases_t
    template <class ENUM>
    ENUM get() {
        if (load_unload_shared_phase < int(ENUM::_init))
            return ENUM::_finish;
        if (load_unload_shared_phase > int(ENUM::_last))
            return ENUM::_finish;
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
    virtual void RestoreTemp() override;
    virtual bool CanSafetyTimerExpire() const override; //evaluate if client can click == safety timer can expire
    virtual void NotifyExpiredFromSafetyTimer(float hotend_temp, float bed_temp) override;
    virtual bool HasTempToRestore() const override;
};

class RammingSequence;

//used by load / unlaod /change filament
class Pause : public PausePrivatePhase {
    pause::Settings settings;
    //singleton
    Pause() = default;
    Pause(const Pause &) = delete;
    Pause &operator=(const Pause &) = delete;

    enum class unload_mode_t {
        standalone,
        standalone_mmu,
        change_filament,
        ask_unloaded,
        autoload_abort // unload from gear - full long unload at high speed
    };
    enum class load_mode_t {
        standalone,
        standalone_mmu,
        change_filament,
        autoload,
        load_in_gear
    };

    static constexpr const float heating_phase_min_hotend_diff = 5.0F;

public:
    static constexpr const float minimal_purge = 1;
    static Pause &Instance();

    bool FilamentUnload(const pause::Settings &settings_);
    bool FilamentUnload_AskUnloaded(const pause::Settings &settings_);
    bool FilamentAutoload(const pause::Settings &settings_);
    bool LoadToGear(const pause::Settings &settings_);
    bool UnloadFromGear(); // does not need config
    bool FilamentLoad(const pause::Settings &settings_);
    bool FilamentLoadNotBlocking(const pause::Settings &settings_);
    void FilamentChange(const pause::Settings &settings_);

private:
    using loop_fn = void (Pause::*)(Response response);
    void loop_unload(Response response);
    void loop_unload_AskUnloaded(Response response);
    void loop_unload_mmu(Response response);
    void loop_unloadFromGear(Response response); //autoload abort
    void loop_unload_change(Response response);
    //TODO loop_unload_change_mmu

    void loop_load(Response response);
    void loop_load_purge(Response response);
    void loop_load_not_blocking(Response response); // no buttons at all - printer without GUI etc
    void loop_load_mmu(Response response);
    void loop_autoload(Response response); //todo force remove filament in retry
    void loop_loadToGear(Response response);
    void loop_load_change(Response response);
    //TODO loop_load_change_mmu

    // does not create FSM_HolderLoadUnload
    bool invoke_loop(loop_fn fn); //shared load/unload code
    bool filamentUnload(loop_fn fn);
    bool filamentLoad(loop_fn fn);

    // park moves calculations
    uint32_t parkMoveZPercent(float z_move_len, float xy_move_len) const;
    uint32_t parkMoveXYPercent(float z_move_len, float xy_move_len) const;
    bool parkMoveXGreaterThanY(const xyz_pos_t &pos0, const xyz_pos_t &pos1) const;

    void unpark_nozzle_and_notify();
    void park_nozzle_and_notify();
    bool is_target_temperature_safe();
    void plan_e_move(const float &length, const feedRate_t &fr_mm_s);
    bool ensureSafeTemperatureNotifyProgress(uint8_t progress_min, uint8_t progress_max);
    void plan_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max);
    void do_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max);
    void do_e_move_notify_progress_coldextrude(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max);
    void do_e_move_notify_progress_hotextrude(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max);
    bool check_user_stop(); //< stops motion and fsm and returns true it user triggered stop
    bool wait_or_stop();    //< waits until motion is finished; if stop is triggered then returns true
    bool process_stop();

    enum class RammingType {
        unload,
        runout
    };

    void ram_filament(const RammingType sequence);
    void unload_filament(const RammingType sequence);
    const RammingSequence &get_ramming_sequence(const RammingType type) const;

    //create finite state machine and automatically destroy it at the end of scope
    //parks in ctor and unparks in dtor
    class FSM_HolderLoadUnload : public FSM_Holder {
        Pause &pause;

        void bindToSafetyTimer();
        void unbindFromSafetyTimer();

    public:
        FSM_HolderLoadUnload(Pause &p, LoadUnloadMode mode);
        ~FSM_HolderLoadUnload();
        friend class Pause;
    };
};
