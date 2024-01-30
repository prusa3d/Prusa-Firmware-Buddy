/**
 * @file filament_sensor.hpp
 * @author Radek Vana
 * @brief basic api of filament sensor
 * @date 2019-12-16
 */

#pragma once

#include <stdint.h>
#include <optional>
#include <atomic>
#include "filament_sensor_states.hpp"
#include "hx717.hpp"

class IFSensor {
    friend class FilamentSensors;

public:
    typedef int32_t value_type;
    static constexpr int32_t undefined_value = HX717::undefined_value;

    enum class Event {
        no_event,
        filament_inserted,
        filament_removed
    };

    virtual ~IFSensor() = default;

    // Evaluates filament sensor state.
    void Cycle();

    // Evaluates if some event happened, will return each event only once, so its meant to be called only internally.
    Event GenerateEvent();

    inline FilamentSensorState get_state() const { return state; }

    /// Returns whether the filament sensor is enabled.
    /// Does not look in the EEPROM/config_store, only respects the sensor current state.
    inline bool is_enabled() const {
        return state != FilamentSensorState::Disabled;
    }

    /**
     * @brief Get filtered sensor-specific value.
     * Useful for sensor info or debug.
     * @return dimensionless value specific to the inherited class
     */
    virtual int32_t GetFilteredValue() const { return 0; };

    // interface methods for sensors with calibration
    // meant to use just flags to be thread safe
    enum class CalibrateRequest {
        CalibrateHasFilament,
        CalibrateNoFilament,
        NoCalibration,
    };
    virtual void SetCalibrateRequest(CalibrateRequest) {}
    virtual bool IsCalibrationFinished() const { return true; }
    virtual void SetInvalidateCalibrationFlag() {}

    virtual void MetricsSetEnabled(bool) {} // Enable/disable metrics for this filament sensor

protected:
    // Protected functions are only to be called from FilamentSensors to prevent race conditions

    FilamentSensorState last_evaluated_state = FilamentSensorState::NotInitialized;
    std::atomic<FilamentSensorState> state = FilamentSensorState::NotInitialized;

    void init();

    virtual void record_state() {}; // record metrics
    virtual void cycle() = 0; // sensor type specific evaluation cycle

    /// Resets the sensor state (sets to disabled if set=false, starts initializing the sensor if true).
    /// Does not change the EEPROM.
    /// This function is not thread-safe.
    virtual void set_enabled(bool set);
};
