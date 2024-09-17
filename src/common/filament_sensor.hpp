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

protected:
    // Protected functions are only to be called from FilamentSensors to prevent race conditions

    std::atomic<FilamentSensorState> state = FilamentSensorState::Disabled;

    /// Records metrics specific for the sensor
    virtual void record_state() {};

    /// sensor type specific evaluation cycle
    virtual void cycle() = 0;

    /// Compares states between this function calls and sets last_event() accordingly.
    void check_for_events();

    /// Returns potential event raised by the last check_for_events() call
    inline Event last_event() const {
        return last_event_;
    }

    /// Resets the sensor state (sets to disabled if set=false, starts initializing the sensor if true).
    /// Resets the state even if you call set_enabled(true) while the sensor is enabled (that is okay).
    /// Does not change the EEPROM.
    /// This function is not thread-safe.
    virtual void force_set_enabled(bool set);

    inline void set_enabled(bool set) {
        if (is_enabled() != set) {
            force_set_enabled(set);
        }
    }

private:
    FilamentSensorState last_check_event_state_ = FilamentSensorState::Disabled;
    Event last_event_ = Event::no_event;
};
