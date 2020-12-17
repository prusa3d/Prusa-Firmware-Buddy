#pragma once
#include "stdint.h"
#include "../../../lib/Marlin/Marlin/src/core/types.h"
#include "client_response.hpp"
#include "marlin_server.hpp"

class PrivatePhase {
    PhasesLoadUnload phase; //needed for CanSafetyTimerExpire
protected:
    PrivatePhase()
        : phase(PhasesLoadUnload::_first) {}
    void setPhase(PhasesLoadUnload ph, uint8_t progress_tot = 0);
    PhasesLoadUnload getPhase() const;
    Response getResponse() const;
    constexpr uint8_t getPhaseIndex() const {
        return GetPhaseIndex(phase);
    }
};

//used by load / unlaod /change filament
class Pause : protected PrivatePhase {
    //singleton
    Pause();
    Pause(const Pause &) = delete;
    Pause &operator=(const Pause &) = delete;

    enum class UnloadPhases_t {
        _init,
        ram_sequence,
        unload,
        unloaded__ask,
        manual_unload,
        _finish
    };

    enum class LoadPhases_t {
        _init = int(UnloadPhases_t::_finish) + 1,
        has_slow_load,
        check_filament_sensor,
        user_push__ask,
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
        _finish
    };

    static constexpr int Z_MOVE_PRECENT = 75;
    static constexpr int XY_MOVE_PRECENT = 100 - Z_MOVE_PRECENT;

    struct RamUnloadSeqItem {
        int16_t e;        ///< relative movement of Extruder
        int16_t feedrate; ///< feedrate of the move
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

    bool CanSafetyTimerExpire() const;

    bool FilamentUnload();
    bool FilamentLoad();
    void FilamentChange();

private:
    bool filamentUnload(); // does not create FSM_HolderLoadUnload
    bool filamentLoad();   // does not create FSM_HolderLoadUnload
    bool loadLoop(LoadPhases_t &load_ph);
    void unloadLoop(UnloadPhases_t &unload_ph);
    void unpark_nozzle_and_notify();
    void park_nozzle_and_notify();
    bool is_target_temperature_safe();
    void hotend_idle_start(uint32_t time);
    void plan_e_move(const float &length, const feedRate_t &fr_mm_s);
    bool ensureSafeTemperatureNotifyProgress(uint8_t progress_min, uint8_t progress_max);
    void plan_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max);
    void do_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max);

    //create finite state machine and automatically destroy it at the end of scope
    //parks in ctor and unparks in dtor
    class FSM_HolderLoadUnload : public FSM_Holder {
        Pause &pause;

    public:
        FSM_HolderLoadUnload(Pause &p, LoadUnloadMode mode)
            : FSM_Holder(ClientFSM::Load_unload, uint8_t(mode))
            , pause(p) {
            pause.park_nozzle_and_notify();
        }

        ~FSM_HolderLoadUnload() {
            pause.unpark_nozzle_and_notify();
        }
        friend class Pause;
    };
};
