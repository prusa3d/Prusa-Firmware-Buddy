// odometer.hpp
#pragma once

#include <stdint.h>
#include <atomic>
#include <utils/utility_extensions.hpp>
#include <inc/MarlinConfig.h>

/// Singleton class that measures
/// distance traveled and filament consumed
class Odometer_s {
public:
    enum class axis_t {
        X,
        Y,
        Z,
        count_
    };
    static constexpr size_t axis_count = ftrstd::to_underlying(axis_t::count_);

private:
    /// stores value changes from the last save
    /// extruder trip counts length of filament used (not moved)
    /// new values are stored to RAM (fast, unlimited writes)
    /// it should be stored to EEPROM after a while (slow, limited number of writes)
    std::atomic<float> trip_xyz[axis_count] = {};
    std::atomic<float> extruded[HOTENDS] = {};
    std::atomic<uint32_t> toolpick[HOTENDS] = {};
    std::atomic<uint32_t> duration_time = 0;

    Odometer_s() = default;

public:
    /// saves values to EEPROM if they are not zero
    void force_to_eeprom();

    /**
     * @brief Are there changes compared to value in EEPROM?
     * @return true when changes exist
     */
    bool changed();

    /**
     * @brief Save new movement.
     * @param axis for this axis
     * @param value distance to add [meters]
     */
    void add_axis(axis_t axis, float value);

    /**
     * @brief Get cumulative movements.
     * @param axis for this axis
     * @return distance moved [meter]
     */
    float get_axis(axis_t axis);

    /**
     * @brief Add extruded length to count.
     * @param extruder for this extruders [indexed from 0]
     * @param value extruded length [meter]
     */
    void add_extruded(uint8_t extruder, float value);

    /**
     * @brief Get extruded length for a particular extruder.
     * @param extruder for this extruder [indexed from 0]
     * @return extruded length [meter]
     */
    float get_extruded(uint8_t extruder);

    /**
     * @brief Get extruded length for all extruders.
     * @return extruded length [meter]
     */
    float get_extruded_all();

    /**
     * @brief Add one toolpick to count.
     * @param extruder for this extruder [indexed from 0]
     */
    void add_toolpick(uint8_t extruder);

    /**
     * @brief Get count of toolchanges for one tool.
     * @param extruder for this extruder [indexed from 0]
     * @return toolpicks for one extruder [1]
     */
    uint32_t get_toolpick(uint8_t extruder);

    /**
     * @brief Get count of all toolchanges.
     * @return sum of toolpicks of all tools [1]
     */
    uint32_t get_toolpick_all();

    /**
     * @brief Save new print duration.
     * @param value print time to accumulate [second]
     */
    void add_time(uint32_t value);

    /**
     * @brief Get print duration.
     * @return print time since eeprom reset [second]
     */
    uint32_t get_time();

    /// Mayer's singleton must have part
public:
    static Odometer_s &instance() {
        static Odometer_s s;
        return s;
    }
    Odometer_s(const Odometer_s &) = delete;
    Odometer_s &operator=(const Odometer_s &) = delete;

private:
    ~Odometer_s() {}
};
