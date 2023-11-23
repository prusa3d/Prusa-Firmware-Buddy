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
#include <option/has_side_fsensor.h>
#include "config_features.h"
#include <option/has_toolchanger.h>

// forward declaration, so I dont need to include freertos
class FreeRTOS_Mutex;

class FilamentSensors {
public:
    FilamentSensors();

    bool HasMMU(); // mmu enabled, might or might not be initialized

    struct BothSensors {
        fsensor_t extruder;
        fsensor_t side;
    };

    BothSensors GetBothSensors();

    fsensor_t GetPrimaryRunout() { return state_of_primary_runout_sensor; };
    fsensor_t GetSecondaryRunout() { return state_of_primary_runout_sensor; };
    fsensor_t GetAutoload() { return state_of_primary_runout_sensor; };
    fsensor_t GetCurrentExtruder() { return state_of_current_extruder; };
    fsensor_t GetCurrentSide() { return state_of_current_side; };
    void Disable();
    void Enable();

    /**
     * @brief Get info from EEPROM if sensors are enabled.
     * @return true if enabled
     */
    bool is_enabled() const;

    enum class EnableResult {
        in_progress, ///< Still trying to enable
        ok, ///< Sensors enabled ok
        not_calibrated, ///< At least one sensor is fsensor_t::NotCalibrated
        not_connected, ///< At least one sensor is fsensor_t::NotConnected
        disabled, ///< Sensors are disabled
    };

    /**
     * @brief Know what happened after enable.
     * @return summed up state of the sensors, error in any sensor returns error
     */
    EnableResult get_enable_result();

    void DisableSideSensor();
    filament_sensor::mmu_enable_result_t EnableSide();
    bool IsSide_processing_request() { return request_side != filament_sensor::cmd_t::null; }
    bool IsExtruderProcessingRequest() { return request_printer != filament_sensor::cmd_t::null; }
    filament_sensor::init_status_t get_active_init_status() const;

    // called from different thread
    void Cycle();

    bool MMUReadyToPrint();
    bool ToolHasFilament(uint8_t tool_nr);

    bool WasM600_send() const { return m600_sent; }

    char GetM600_send_on() const;

    uint32_t DecEvLock();
    uint32_t IncEvLock();

    uint32_t DecAutoloadLock();
    uint32_t IncAutoloadLock();

    // calling clear of m600 and autoload flags is safe from any thread, but setting them would not be !!!
    void ClrM600Sent() { m600_sent = false; }
    void ClrAutoloadSent() { autoload_sent = false; }
    bool IsAutoloadInProgress() { return autoload_sent; }
    MMU2::FilamentState WhereIsFilament();

    void AdcExtruder_FilteredIRQ(int32_t val, uint8_t tool_index); // ADC sensor IRQ callback
    void AdcSide_FilteredIRQ(int32_t val, uint8_t tool_index); // ADC sensor IRQ callback

    static constexpr bool IsWorking(fsensor_t sens) { return sens == fsensor_t::HasFilament || sens == fsensor_t::NoFilament; }

private:
    void SetToolIndex();

    void configure_sensors();
    void reconfigure_sensors_if_needed(); // for some printers might do something else than configure_sensors
    void set_corresponding_variables();

    filament_sensor::Events evaluate_logical_sensors_events();

    bool evaluateM600(FSensor::event ev) const; // must remain const - is called out of critical section
    bool evaluateAutoload(FSensor::event ev) const; // must remain const - is called out of critical section
    inline bool isEvLocked() { return event_lock > 0; }
    inline bool isAutoloadLocked() { return autoload_lock > 0; }

    bool has_mmu2_enabled() const; // turn MMU2 on during init

    void process_printer_request();
    void all_sensors_initialized();
    void process_side_request();
    bool run_sensors_cycle(); //< returns true,

    // logical sensors
    // 1 physical sensor can be linked to multiple logical sensors
    filament_sensor::LogicalSensors logical_sensors;

    // all those variables can be accessed from multiple threads
    // all of them are set during critical section, so values are guaranteed to be corresponding
    // in case multiple values are needed they should be read during critical section too
    std::atomic<uint32_t> event_lock; // 0 == unlocked
    std::atomic<uint32_t> autoload_lock; // 0 == unlocked
    std::atomic<fsensor_t> state_of_primary_runout_sensor = fsensor_t::NotInitialized; // We need those. States obtained from from sensors directly might not by synchronized
    std::atomic<fsensor_t> state_of_secondary_runout_sensor = fsensor_t::NotInitialized;
    std::atomic<fsensor_t> state_of_autoload_sensor = fsensor_t::NotInitialized;

    std::atomic<fsensor_t> state_of_current_extruder = fsensor_t::NotInitialized;
    std::atomic<fsensor_t> state_of_current_side = fsensor_t::NotInitialized;

    std::atomic<filament_sensor::init_status_t> init_status = filament_sensor::init_status_t::NotReady; // combined status of all sensors, must be invalidated when tool_index changes

    std::atomic<filament_sensor::cmd_t> request_side = filament_sensor::cmd_t::null;
    std::atomic<filament_sensor::cmd_t> request_printer = filament_sensor::cmd_t::null;

    std::atomic<uint8_t> tool_index = 0;
    std::atomic<bool> m600_sent = false;
    std::atomic<bool> autoload_sent = false;
    std::atomic<bool> has_mmu = false; // affect only MMU, named correctly .. it is not "has_side_sensor"

    // I have used reference to forward declared class, so I do not need to include freertos in header
    FreeRTOS_Mutex &GetSideMutex();
    FreeRTOS_Mutex &GetExtruderMutex();

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
