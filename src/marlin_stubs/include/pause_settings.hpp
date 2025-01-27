/**
 * @file pause_settings.hpp
 * @brief
 */

#pragma once
#include <optional>
#include "../../../lib/Marlin/Marlin/src/core/types.h"

class Pause; // forward declaration, so Settings does not think Pause is member of pause namespace

namespace pause {

class Settings {
public:
    enum class CalledFrom : uint_least8_t {
        Pause,
        FilamentStuck
    };

    Settings();
    static constexpr const float minimal_purge = 1;

    // defaults
    static float GetDefaultFastLoadLength();
    static float GetDefaultSlowLoadLength();
    static float GetDefaultUnloadLength();
    static float GetDefaultPurgeLength();
    static float GetDefaultRetractLength();
    static float GetDefaultParkZFeedrate(); ///< Get feedrate for park z move [mm/s].

    void SetUnloadLength(const std::optional<float> &len);
    void SetSlowLoadLength(const std::optional<float> &len);
    void SetFastLoadLength(const std::optional<float> &len);
    void SetPurgeLength(const std::optional<float> &len);
    void SetRetractLength(const std::optional<float> &len);
    void SetParkZFeedrate(const std::optional<float> &feedrate); ///< Set feedrate for park z move [mm/s].
    void SetParkPoint(const xyz_pos_t &park_point);
    void SetResumePoint(const xyze_pos_t &resume_point);
    void SetMmuFilamentToLoad(uint8_t index);

    void SetExtruder(uint8_t target) { target_extruder = target; }
    uint8_t GetExtruder() const { return target_extruder; }

    void SetCalledFrom(CalledFrom cf) { called_from = cf; }
    CalledFrom GetCalledFrom() const { return called_from; }

private:
    friend class ::Pause; // forward declaration of Pause is not enough, have to add scope resolution operator too

    // this values must be set before every load/unload
    float unload_length;
    float slow_load_length;
    float fast_load_length;
    float purge_length;
    float retract;
    float park_z_feedrate; ///< feedrate for park z move [mm/s]

    xyz_pos_t park_pos; // if axis is NAN, don't move it
    xyze_pos_t resume_pos;

    uint8_t mmu_filament_to_load = 0;
    uint8_t target_extruder;
    bool can_stop : 1; // true by default, only runout cannot stop, set by Pause
    bool do_stop : 1; // part of settings just o be resetted

    // Preloaded from the config_store to prevent querying it each loop
    bool extruder_mmu_rework : 1 = false;

    CalledFrom called_from = CalledFrom::Pause;
};

} // namespace pause
