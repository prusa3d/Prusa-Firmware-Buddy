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
public:
    typedef int32_t value_type;
    static constexpr int32_t undefined_value = HX717::undefined_value;

    enum class event {
        NoFilament,
        HasFilament,
        EdgeFilamentInserted,
        EdgeFilamentRemoved
    };
    virtual ~IFSensor() = default;

    // Evaluates filament sensor state.
    void Cycle();

    // Evaluates if some event happened, will return each event only once, so its meant to be called only internally.
    event GenerateEvent();

    // thread safe functions
    fsensor_t Get() { return state; }

    /**
     * @brief Get filtered sensor-specific value.
     * Useful for sensor info or debug.
     * @return dimensionless value specific to the inherited class
     */
    virtual int32_t GetFilteredValue() const { return 0; };

    // thread safe functions, but cannot be called from interrupt
    void Enable();
    void Disable();

    // interface methods for sensors with calibration
    // meant to use just flags to be thread safe
    enum class CalibrateRequest {
        CalibrateHasFilament,
        CalibrateNoFilament,
        NoCalibration,
    };
    virtual void SetCalibrateRequest(CalibrateRequest) {}
    virtual bool IsCalibrationFinished() const { return true; }
    virtual void SetLoadSettingsFlag() {}
    virtual void SetInvalidateCalibrationFlag() {}

    virtual void MetricsSetEnabled(bool) {} // Enable/disable metrics for this filament sensor

protected:
    fsensor_t last_evaluated_state = fsensor_t::NotInitialized;
    std::atomic<fsensor_t> state = fsensor_t::NotInitialized;

    virtual void record_state() = 0; // record metrics
    virtual void cycle() = 0; // sensor type specific evaluation cycle
    virtual void enable() = 0; // enables sensor called from Enable(), does not have locks
    virtual void disable() = 0; // disables sensor called from Disable(), does not have locks
};

// basic filament sensor api
class FSensor : public IFSensor {
protected:
    std::atomic<fsensor_t> last_state = fsensor_t::NotInitialized;

    void init();
    virtual void set_state(fsensor_t st) = 0;

public:
    // thread safe function, but cannot be called from interrupt
    fsensor_t WaitInitialized();
};
