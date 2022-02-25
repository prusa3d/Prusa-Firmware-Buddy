/**
 * @file pause_stubbed.hpp
 * @author Radek Vana
 * @brief stubbed version of marlin pause.hpp
 * mainly used for load / unload / change filament
 * @date 2020-12-18
 */

#pragma once
#include "stdint.h"
#include "../../../lib/Marlin/Marlin/src/core/types.h"
#include "client_response.hpp"
#include "marlin_server.hpp"
#include "IPause.hpp"
#include <array>

class PausePrivatePhase : public IPause {
    PhasesLoadUnload phase;       //needed for CanSafetyTimerExpire
    int load_unload_shared_phase; //shared variable for UnloadPhases_t and LoadPhases_t

    float nozzle_restore_temp;
    float bed_restore_temp;

protected:
    enum class UnloadPhases_t {
        _init,
        ram_sequence,
        unload,
        unloaded__ask,
        manual_unload,
        filament_not_in_fs,
        _phase_does_not_exist,
        _finish = _phase_does_not_exist
    };

    enum class LoadPhases_t {
        _init = int(UnloadPhases_t::_finish) + 1,
        has_slow_load,
        check_filament_sensor_and_user_push__ask, //must be one phase because of button click
        load_in_gear,
        wait_temp,
        error_temp,
        has_long_load,
        long_load,
        purge,
        stand_alone_purge,
        ask_is_color_correct,
        ask_is_color_correct__stand_alone_purge,
        eject,
        _phase_does_not_exist,
        _finish = _phase_does_not_exist
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
            return ENUM::_phase_does_not_exist;
        if (load_unload_shared_phase > int(ENUM::_finish))
            return ENUM::_phase_does_not_exist;
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
    //singleton
    Pause();
    Pause(const Pause &) = delete;
    Pause &operator=(const Pause &) = delete;

    enum class unload_mode_t {
        standalone,
        change_filament,
        ask_unloaded
    };
    enum class load_mode_t {
        standalone,
        change_filament
    };

    static constexpr const float heating_phase_min_hotend_diff = 5.0F;

    //this values must be set before every load/unload
    float unload_length;
    float slow_load_length;
    float fast_load_length;
    float purge_length = minimal_purge;
    float retract;

    xyz_pos_t park_pos;
    xyze_pos_t resume_pos;

public:
    static constexpr const float minimal_purge = 1;
    static Pause &Instance();

    //defaults
    static float GetDefaultFastLoadLength();
    static float GetDefaultSlowLoadLength();
    static float GetDefaultUnloadLength();
    static float GetDefaultPurgeLength();
    static float GetDefaultRetractLength();

    void SetUnloadLength(float len);
    void SetSlowLoadLength(float len);
    void SetFastLoadLength(float len);
    void SetPurgeLength(float len);
    void SetRetractLength(float len);
    void SetParkPoint(const xyz_pos_t &park_point);
    void SetResumePoint(const xyze_pos_t &resume_point);

    bool FilamentUnload();
    bool FilamentUnload_AskUnloaded();
    bool FilamentLoad();
    void FilamentChange();

private:
    // park moves calculations
    uint32_t parkMoveZPercent(float z_move_len, float xy_move_len) const;
    uint32_t parkMoveXYPercent(float z_move_len, float xy_move_len) const;
    bool parkMoveXGreaterThanY(const xyz_pos_t &pos0, const xyz_pos_t &pos1) const;

    bool filamentUnload(unload_mode_t mode); // does not create FSM_HolderLoadUnload
    bool filamentLoad(load_mode_t mode);     // does not create FSM_HolderLoadUnload
    bool loadLoop(load_mode_t mode);
    void unloadLoop(unload_mode_t mode);
    void unpark_nozzle_and_notify();
    void park_nozzle_and_notify();
    bool is_target_temperature_safe();
    void plan_e_move(const float &length, const feedRate_t &fr_mm_s);
    bool ensureSafeTemperatureNotifyProgress(uint8_t progress_min, uint8_t progress_max);
    void plan_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max);
    void do_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max);
    void do_e_move_notify_progress_coldextrude(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max);
    void do_e_move_notify_progress_hotextrude(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max);

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
