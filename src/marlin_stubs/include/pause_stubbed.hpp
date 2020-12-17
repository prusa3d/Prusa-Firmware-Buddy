#pragma once
#include "stdint.h"
#include "../../../lib/Marlin/Marlin/src/core/types.h"
#include "client_response.hpp"

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
    Pause() {}
    Pause(const Pause &) = delete;
    Pause &operator=(const Pause &) = delete;

    enum class LoadPhases_t {
        _init,
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

    enum class UnloadPhases_t {
        _init,
        ram_sequence,
        unload,
        unloaded__ask,
        manual_unload,
        _finish
    };

    static constexpr int Z_MOVE_PRECENT = 75;
    static constexpr int XY_MOVE_PRECENT = 100 - Z_MOVE_PRECENT;

    struct RamUnloadSeqItem {
        int16_t e;        ///< relative movement of Extruder
        int16_t feedrate; ///< feedrate of the move
    };

    static constexpr const float heating_phase_min_hotend_diff = 5.0F;

    xyze_pos_t resume_position;

    //this values must be set before every load/unload
    float unload_length = 0;
    float slow_load_length = 0;
    float fast_load_length = 0;
    float purge_length = minimal_purge;

public:
    static constexpr const float minimal_purge = 1;
    static Pause &GetInstance();

    void SetUnloadLength(float len);
    void SetSlowLoadLength(float len);
    void SetFastLoadLength(float len);
    void SetPurgeLength(float len);
    float GetDefaultLoadLength() const;
    float GetDefaultUnloadLength() const;
    bool FilamentUnload();
    bool FilamentLoad();
    bool PrintPause(float retract, const xyz_pos_t &park_point);
    void PrintResume();
    bool CanSafetyTimerExpire() const;

private:
    bool loadLoop(LoadPhases_t &load_ph);
    void unloadLoop(UnloadPhases_t &unload_ph);
    void unpark_nozzle_and_notify();
    void park_nozzle_and_notify(const float &retract, const xyz_pos_t &park_point);
    bool is_target_temperature_safe();
    void hotend_idle_start(uint32_t time);
    void plan_e_move(const float &length, const feedRate_t &fr_mm_s);
    bool ensureSafeTemperatureNotifyProgress(uint8_t progress_min, uint8_t progress_max);
    void plan_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max);
    void do_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, uint8_t progress_min, uint8_t progress_max);
};

extern Pause &pause;
