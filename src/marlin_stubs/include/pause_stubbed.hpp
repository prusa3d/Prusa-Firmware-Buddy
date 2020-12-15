#pragma once
#include "stdint.h"
#include "../../../lib/Marlin/Marlin/src/core/types.h"
#include "client_response.hpp"

//used by load / unlaod /change filament
class Pause {
    xyze_pos_t resume_position;

    //singleton
    Pause() {}
    Pause(const Pause &) = delete;
    Pause &operator=(const Pause &) = delete;

    enum { Z_MOVE_PRECENT = 75,
        XY_MOVE_PRECENT = 100 - Z_MOVE_PRECENT };

    struct RamUnloadSeqItem {
        int16_t e;        ///< relative movement of Extruder
        int16_t feedrate; ///< feedrate of the move
    };

    static constexpr const float heating_phase_min_hotend_diff = 5.0F;

    //this walues must be set before every load/unload
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
    bool CanSafetyTimerExpire() { return true; }

private:
    void unpark_nozzle_and_notify();
    void park_nozzle_and_notify(const float &retract, const xyz_pos_t &park_point);
    bool is_target_temperature_safe();
    bool ensure_safe_temperature_notify_progress(PhasesLoadUnload phase, uint8_t progress_min, uint8_t progress_max);
    void hotend_idle_start(uint32_t time);
    void plan_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, PhasesLoadUnload phase, uint8_t progress_min, uint8_t progress_max);
    void plan_e_move(const float &length, const feedRate_t &fr_mm_s);
    void do_e_move_notify_progress(const float &length, const feedRate_t &fr_mm_s, PhasesLoadUnload phase, uint8_t progress_min, uint8_t progress_max);
};

extern Pause &pause;
