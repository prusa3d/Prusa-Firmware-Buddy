// odometer.hpp
#pragma once

#include <stdint.h>
#include <atomic>
#include <utils/utility_extensions.hpp>
#include <inc/MarlinConfig.h>
#include <tool_index.hpp>
#include <option/has_wastebin_fill_tracking.h>

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
    static constexpr size_t axis_count = std::to_underlying(axis_t::count_);

private:
    /// stores value changes from the last save
    /// extruder trip counts length of filament used (not moved)
    /// new values are stored to RAM (fast, unlimited writes)
    /// it should be stored to EEPROM after a while (slow, limited number of writes)
    std::atomic<float> trip_xyz[axis_count] = {};
    StrongIndexArray<std::atomic<float>, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> extruded {};
    StrongIndexArray<std::atomic<uint32_t>, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> toolpick {};
    std::atomic<uint32_t> duration_time = 0;
    std::atomic<uint32_t> mmu_changes = 0;
#if HAS_WASTEBIN_FILL_TRACKING()
    /// Pellets ejected into the INDX nozzle-cleaner wastebin since last emptied (RAM delta, flushed to eeprom).
    /// The fill-tracking policy on top of this lives in WastebinWatcher.
    std::atomic<uint32_t> nozzle_cleaner_pellets = 0;
#endif

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
     * @param value distance to add [mm]
     */
    void add_axis(axis_t axis, float value);

    /**
     * @brief Get cumulative movements.
     * @param axis for this axis
     * @return distance moved [mm]
     */
    float get_axis(axis_t axis);

    /**
     * @brief Add extruded length to count.
     * @param tool target tool
     * @param value extruded length [mm]
     */
    void add_extruded(PhysicalToolIndex tool, float value);

    /**
     * @brief Get extruded length for a particular tool.
     * @param tool target tool
     * @return extruded length [mm]
     */
    float get_extruded(PhysicalToolIndex tool);

    /**
     * @brief Get extruded length for all extruders.
     * @return extruded length [mm]
     */
    float get_extruded_all();

    /**
     * @brief Add one toolpick to count.
     * @param tool target tool
     */
    void add_toolpick(PhysicalToolIndex tool);

    /**
     * @brief Get count of toolchanges for one tool.
     * @param tool target tool
     * @return toolpick count [1]
     */
    uint32_t get_toolpick(PhysicalToolIndex tool);

    /**
     * @brief Get count of all toolchanges.
     * @return sum of toolpicks of all tools [1]
     */
    uint32_t get_toolpick_all();

    /**
     * @brief Add one MMU filament change.
     */
    void add_mmu_change();

    /**
     * @brief Get count of MMU filament changes.
     */
    uint32_t get_mmu_changes();

#if HAS_WASTEBIN_FILL_TRACKING()
    /// Register one pellet ejected into the nozzle-cleaner wastebin.
    void add_nozzle_cleaner_pellet();

    /// Get the number of pellets in the wastebin since it was last emptied.
    uint32_t get_nozzle_cleaner_pellets();

    /// Empty the wastebin: reset the pellet counter (RAM accumulator + persisted value) to zero.
    void reset_nozzle_cleaner_pellets();
#endif

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
        return instance_;
    }
    Odometer_s(const Odometer_s &) = delete;
    Odometer_s &operator=(const Odometer_s &) = delete;

private:
    ~Odometer_s() {}

    static Odometer_s instance_;
};
