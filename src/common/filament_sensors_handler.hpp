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
#include "config_features.h"
#include <option/has_toolchanger.h>

#include <inplace_function.hpp>

/// Filament sensors manager
/// All public functions are thread-safe
/// All other functions can be only called from process()
class FilamentSensors {
public:
    FilamentSensors();

    /// Sets global filament sensor enable
    void set_enabled_global(bool set);

    /// Sends a request to the fsensor task
    /// to update the sensors enable/disable state based on EEPROM settings
    /// \param check_fs [in] check if FS is enabled and turn off MMU if it is not
    void request_enable_state_update(bool check_fs = true);

    /// Returns whether fsensors enable state update was requested and is not yet fully processed
    inline bool is_enable_state_update_processing() const {
        return enable_state_update_pending || enable_state_update_processing;
    }

    /// !!! To be called only from the GUI thread
    /// Blockingly waits till the newly enabled sensors get initialized.
    /// Pops up a warning if the newly enabled sensors need calibration or are disconnected.
    /// \returns if everything was okay
    bool gui_wait_for_init_with_msg();

    /// Calls \p f on all filament sensors
    void for_all_sensors(const stdext::inplace_function<void(IFSensor &sensor, uint8_t index, bool is_side)> &f);

    // mmu enabled, might or might not be initialized
    inline bool HasMMU() const {
        return has_mmu;
    }
    bool MMUReadyToPrint();
    bool ToolHasFilament(uint8_t tool_nr);

    void DecEvLock();
    void IncEvLock();

    void DecAutoloadLock();
    void IncAutoloadLock();

    // calling clear of m600 and autoload flags is safe from any thread, but setting them would not be !!!
    void ClrM600Sent() { m600_sent = false; }
    void ClrAutoloadSent() { autoload_sent = false; }
    bool IsAutoloadInProgress() { return autoload_sent; }
    MMU2::FilamentState WhereIsFilament();

    /// Thread-safe
    inline IFSensor *sensor(LogicalFilamentSensor sensor) const {
        return logical_sensors_[sensor];
    }

    /// Thread-safe
    inline FilamentSensorState sensor_state(LogicalFilamentSensor sensor) const {
        return logical_sensor_states_[sensor];
    }

    /// \returns whether the printer knows that it HAS the filament
    /// If the filament sensor is disabled, not calibrated, disconnected and such, always returns false
    inline bool has_filament_surely() {
        return logical_sensor_states_[LogicalFilamentSensor::current_extruder] == FilamentSensorState::HasFilament;
    }

    /// \returns whether the printer knows that it HASN'T the filament
    /// If the filament sensor is disabled, not calibrated, disconnected and such, always returns false
    inline bool no_filament_surely() {
        return logical_sensor_states_[LogicalFilamentSensor::current_extruder] == FilamentSensorState::NoFilament;
    }

protected:
    /// For some historical reason, filament sensor runs in the context of measurement task
    friend void StartMeasurementTask(void const *);

    /// Called from measurement task once
    void task_init();

    /// Periodically called from the measurement task
    void task_cycle();

private:
    // Non-public members can only be written to from the cycle() function (called from the Measurement task)
    // The variables are made atomic so that one can read them from different threads and get somewhat valid values.

    void reconfigure_sensors_if_needed(bool force);
    void process_events();
    void process_enable_state_update();

    inline bool isEvLocked() const { return event_lock > 0; }
    inline bool isAutoloadLocked() const { return autoload_lock > 0; }

    // logical sensors
    // 1 physical sensor can be linked to multiple logical sensors
    LogicalFilamentSensors logical_sensors_;

    LogicalFilamentSensorStates logical_sensor_states_;

    // all those variables can be accessed from multiple threads
    // all of them are set during critical section, so values are guaranteed to be corresponding
    // in case multiple values are needed they should be read during critical section too
    std::atomic<uint8_t> event_lock; // 0 == unlocked
    std::atomic<uint8_t> autoload_lock; // 0 == unlocked

    /// If set, the fsensors enable/disable states
    /// will be reconfigured in the next fsensors update cycle
    std::atomic<bool> enable_state_update_pending = false;
    std::atomic<bool> enable_state_update_processing = false;

    std::atomic<uint8_t> tool_index = uint8_t(-1);
    std::atomic<bool> m600_sent = false;
    std::atomic<bool> autoload_sent = false;
    std::atomic<bool> has_mmu = false; // affect only MMU, named correctly .. it is not "has_side_sensor"

    friend IFSensor *GetExtruderFSensor(uint8_t index);
    friend IFSensor *GetSideFSensor(uint8_t index);
};

// singleton
FilamentSensors &FSensors_instance();

IFSensor *GetExtruderFSensor(uint8_t index);
IFSensor *GetSideFSensor(uint8_t index);

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
