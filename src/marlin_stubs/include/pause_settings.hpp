/**
 * @file pause_settings.hpp
 * @brief
 */

#pragma once
#include <optional>
#include "../../../lib/Marlin/Marlin/src/core/macros.h"
#include "../../../lib/Marlin/Marlin/src/core/types.h"

class Pause; //forward declaration, so Settings does not think Pause is member of pause namespace

namespace pause {

class Settings {
    friend class ::Pause; // forward declaration of Pause is not enough, have to add scope resolution operator too

    //this values must be set before every load/unload
    float unload_length;
    float slow_load_length;
    float fast_load_length;
    float purge_length;
    float retract;

    xyz_pos_t park_pos; // if axis is NAN, don't move it
    xyze_pos_t resume_pos;

    uint8_t mmu_filament_to_load = 0;
    uint8_t target_extruder;
    bool can_stop; // true by default, only runout cannot stop, set by Pause
    bool do_stop;  // part of settings just o be resetted

public:
    Settings();
    static constexpr const float minimal_purge = 1;

    //defaults
    static float GetDefaultFastLoadLength();
    static float GetDefaultSlowLoadLength();
    static float GetDefaultUnloadLength();
    static float GetDefaultPurgeLength();
    static float GetDefaultRetractLength();

    void SetUnloadLength(const std::optional<float> &len);
    void SetSlowLoadLength(const std::optional<float> &len);
    void SetFastLoadLength(const std::optional<float> &len);
    void SetPurgeLength(const std::optional<float> &len);
    void SetRetractLength(const std::optional<float> &len);
    void SetParkPoint(const xyz_pos_t &park_point);
    void SetResumePoint(const xyze_pos_t &resume_point);
    void SetMmuFilamentToLoad(uint8_t index);

    void SetExtruder(uint8_t target) { target_extruder = target; }
    uint8_t GetExtruder() const { return target_extruder; }
};

} // namespace pause
