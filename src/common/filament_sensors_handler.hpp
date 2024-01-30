/**
 * @file filament_sensors_handler.hpp
 * @brief api (facade) handling printer and MMU filament sensors
 * it cannot be used in ISR
 */

#pragma once

#include "stdint.h"
#include "filament_sensor.hpp"
#include "filament_sensor_types.hpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_fsensor.h" // MMU2::FilamentState
#include <atomic>
#include <bitset>
#include <option/has_side_fsensor.h>
#include "config_features.h"
#include <option/has_toolchanger.h>

// forward declaration, so I dont need to include freertos
namespace freertos {
class Mutex;
}

class FilamentSensors {
public:
    FilamentSensors();

    bool HasMMU(); // mmu enabled, might or might not be initialized

    FilamentSensorState GetPrimaryRunout() { return state_of_primary_runout_sensor; };
    FilamentSensorState GetSecondaryRunout() { return state_of_primary_runout_sensor; };
    FilamentSensorState GetAutoload() { return state_of_primary_runout_sensor; };
    FilamentSensorState GetCurrentExtruder() { return state_of_current_extruder; };
    FilamentSensorState GetCurrentSide() { return state_of_current_side; };

    /// Sets global filament sensor enable
    void set_enabled_global(bool set);

    /// Sends a request to the fsensor task
    /// to update the sensors enable/disable state based on EEPROM settings
    void request_enable_state_update();

    /// Returns whether fsensors enable state update was requested and is not yet fully processed
    inline bool is_enable_state_update_processing() const {
        return enable_state_update_pending || enable_state_update_processing;
    }

    /// Calls \p f on all filament sensors
    void for_all_sensors(const std::function<void(IFSensor &)> &f);

    // called from different thread
    void Cycle();

    bool MMUReadyToPrint();
    bool ToolHasFilament(uint8_t tool_nr);

    bool WasM600_send() const { return m600_sent; }

    char GetM600_send_on() const;

    void DecEvLock();
    void IncEvLock();

    void DecAutoloadLock();
    void IncAutoloadLock();

    // calling clear of m600 and autoload flags is safe from any thread, but setting them would not be !!!
    void ClrM600Sent() { m600_sent = false; }
    void ClrAutoloadSent() { autoload_sent = false; }
    bool IsAutoloadInProgress() { return autoload_sent; }
    MMU2::FilamentState WhereIsFilament();

    void AdcExtruder_FilteredIRQ(int32_t val, uint8_t tool_index); // ADC sensor IRQ callback
    void AdcSide_FilteredIRQ(int32_t val, uint8_t tool_index); // ADC sensor IRQ callback

private:
    void reconfigure_sensors_if_needed(bool force);
    void set_corresponding_variables();

    filament_sensor::Events evaluate_logical_sensors_events();

    bool evaluateM600(std::optional<IFSensor::Event> ev) const; // must remain const - is called out of critical section
    bool evaluateAutoload(std::optional<IFSensor::Event> ev) const; // must remain const - is called out of critical section
    inline bool isEvLocked() const { return event_lock > 0; }
    inline bool isAutoloadLocked() const { return autoload_lock > 0; }

    // logical sensors
    // 1 physical sensor can be linked to multiple logical sensors
    filament_sensor::LogicalSensors logical_sensors;

    // all those variables can be accessed from multiple threads
    // all of them are set during critical section, so values are guaranteed to be corresponding
    // in case multiple values are needed they should be read during critical section too
    std::atomic<uint8_t> event_lock; // 0 == unlocked
    std::atomic<uint8_t> autoload_lock; // 0 == unlocked
    std::atomic<FilamentSensorState> state_of_primary_runout_sensor = FilamentSensorState::NotInitialized; // We need those. States obtained from from sensors directly might not by synchronized
    std::atomic<FilamentSensorState> state_of_secondary_runout_sensor = FilamentSensorState::NotInitialized;
    std::atomic<FilamentSensorState> state_of_autoload_sensor = FilamentSensorState::NotInitialized;

    std::atomic<FilamentSensorState> state_of_current_extruder = FilamentSensorState::NotInitialized;
    std::atomic<FilamentSensorState> state_of_current_side = FilamentSensorState::NotInitialized;

    /// If set, the fsensors enable/disable states
    /// will be reconfigured in the next fsensors update cycle
    std::atomic<bool> enable_state_update_pending = false;
    std::atomic<bool> enable_state_update_processing = false;

    void process_enable_state_update();

    std::atomic<uint8_t> tool_index = uint8_t(-1);
    std::atomic<bool> m600_sent = false;
    std::atomic<bool> autoload_sent = false;
    std::atomic<bool> has_mmu = false; // affect only MMU, named correctly .. it is not "has_side_sensor"

    // I have used reference to forward declared class, so I do not need to include freertos in header
    freertos::Mutex &GetExtruderMutex();

    friend IFSensor *GetExtruderFSensor(uint8_t index);
    friend IFSensor *GetSideFSensor(uint8_t index);
    friend IFSensor *get_active_printer_sensor();

    friend IFSensor *get_active_side_sensor();
};

// singleton
FilamentSensors &FSensors_instance();

IFSensor *GetExtruderFSensor(uint8_t index);

IFSensor *GetSideFSensor(uint8_t index);

IFSensor *get_active_printer_sensor();

IFSensor *get_active_side_sensor();

/**
 * @brief called from IRQ
 * it is super important to pass index of extruder too
 * to prevent sending data to wrong sensor
 * it could cause false runout!!!
 *
 * @param fs_raw_value sample value
 */
void fs_process_sample(int32_t fs_raw_value, uint8_t tool_index);

/**
 * @brief called from IRQ
 * it is super important to pass index of extruder too
 * to prevent sending data to wrong sensor
 * it could cause false runout!!!
 *
 * @param fs_raw_value sample value
 */
void side_fs_process_sample(int32_t fs_raw_value, uint8_t tool_index);
