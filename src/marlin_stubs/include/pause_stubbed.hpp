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

public:
    static Pause &GetInstance();

    float GetLoadLength() const;
    float GetUnloadLength() const;
    bool FilamentUnload(const float &unload_length);
    bool FilamentLoad(const float &slow_load_length, const float &fast_load_length, const float &purge_length);
    bool PrintPause(const float &retract, const xyz_pos_t &park_point, const float &unload_length);
    void PrintResume(const float &slow_load_length, const float &fast_load_length, const float &purge_length);

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
