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
#include <eeprom.h>

enum class fsensor_t : uint8_t {
    NotInitialized, //enable enters this state too
    NotCalibrated,
    HasFilament,
    NoFilament,
    NotConnected,
    Disabled
};

class IFSensor {
public:
    enum class event {
        NoFilament,
        HasFilament,
        EdgeFilamentInserted,
        EdgeFilamentRemoved
    };

    std::optional<event> Cycle();

    //thread safe functions
    fsensor_t Get() { return state; }

    /**
     * @brief Get filtered sensor-specific value.
     * Useful for sensor info or debug.
     * @return dimensionless value specific to the inherited class
     */
    virtual int32_t GetFilteredValue() const { return 0; };

    //thread safe functions, but cannot be called from interrupt
    void Enable();
    void Disable();

    //interface methods for sensors with calibration
    //meant to use just flags to be thread safe
    virtual void SetCalibrateFlag() {}
    virtual void SetLoadSettingsFlag() {}
    virtual void SetInvalidateCalibrationFlag() {}

    virtual eevar_id get_eeprom_span_id() const { return eevar_id::EEVAR_CRC32; } // return crc as error
    virtual eevar_id get_eeprom_ref_id() const { return eevar_id::EEVAR_CRC32; }  // return crc as error
protected:
    std::atomic<fsensor_t> state = fsensor_t::NotInitialized;
    event generateEvent(fsensor_t last_state_before_cycle) const;

    virtual void record_state() = 0; // record metrics
    virtual void cycle() = 0;        // sensor type specific evaluation cycle
    virtual void enable() = 0;       // enables sensor called from Enable(), does not have locks
    virtual void disable() = 0;      // disables sensor called from Disable(), does not have locks
};

//basic filament sensor api
class FSensor : public IFSensor {
protected:
    std::atomic<fsensor_t> last_state = fsensor_t::NotInitialized;

    void init();
    virtual void set_state(fsensor_t st) = 0;

public:
    //thread safe function, but cannot be called from interrupt
    fsensor_t WaitInitialized();
};
