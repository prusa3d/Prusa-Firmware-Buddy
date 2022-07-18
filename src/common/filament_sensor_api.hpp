/**
 * @file filament_sensor_api.hpp
 * @brief api (facade) handling printer and MMU filament sensors
 * it cannot be used in ISR
 */

#pragma once

#include "stdint.h"
#include "filament_sensor.hpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa_MMU2/mmu2_fsensor.h" // MMU2::FilamentState
#include <atomic>

// forward declaration, so I dont need to include freertos
class FreeRTOS_Mutex;

class FilamentSensors {
private:
    // I have used reference to forward decladed class, so I do not need to include freertos in header
    FreeRTOS_Mutex &mutex_mmu;
    FreeRTOS_Mutex &mutex_printer;

public:
    FilamentSensors(FreeRTOS_Mutex &mmu, FreeRTOS_Mutex &printer);

    // keep enums 32 bit
    // so atomic will probably just use volatile
    enum class inject_t {
        on_edge = 0,
        on_level = 1,
        never = 2
    };

    enum class cmd_t {
        null,
        on,
        off,
        processing
    };

    enum class init_status_t {
        Ok,
        PrinterNotInitialized,
        MmuNotInitialized,
        BothNotInitialized,
        NotReady
    };

    enum class mmu_enable_result_t {
        ok,
        error_filament_sensor_disabled,
        error_mmu_not_supported
    };

    bool HasMMU(); // mmu enabled, might or might not be initialized
    fsensor_t Get();

    void Disable();
    void Enable();
    void DisableMMU();
    mmu_enable_result_t EnableMMU();
    bool IsMMU_processing_request() { return request_mmu != cmd_t::null; }
    bool IsPrinter_processing_request() { return request_printer != cmd_t::null; }
    init_status_t GetInitStatus() const { return init_status; }
    fsensor_t GetPrinter();
    fsensor_t GetMMU();

    //called from different thread
    void Cycle();

    bool CanStartPrint(); //has_filament != mmu_active

    void M600_on_edge() { send_event_on = inject_t::on_edge; }

    void M600_on_level() { send_event_on = inject_t::on_level; }

    void M600_never() { send_event_on = inject_t::never; }

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

    //simpler api functions
    //HasFilament != !HasNotFilament, there is many more states
    inline bool HasFilament() { return Get() == fsensor_t::HasFilament; }
    inline bool HasNotFilament() { return Get() == fsensor_t::NoFilament; }
    inline bool IsWorking() { return IsWorking(Get()); }
    static constexpr bool IsWorking(fsensor_t sens) { return sens == fsensor_t::HasFilament || sens == fsensor_t::NoFilament; }

    inline bool PrinterHasFilament() { return state_of_printer_sensor == fsensor_t::HasFilament; }

private:
    fsensor_t get() const;
    bool evaluateM600(FSensor::event ev) const;     // must remain const - is called out of critical section
    bool evaluateAutoload(FSensor::event ev) const; // must remain const - is called out of critical section
    inline bool isEvLocked() { return event_lock > 0; }
    inline bool isAutoloadLocked() { return autoload_lock > 0; }

    void restore_send_M600_on(inject_t send_event_on);
    inject_t getM600_send_on_and_disable();

    void mmu_disable();
    void mmu_enable();

    void printer_disable();
    void printer_enable();

    void wait_printer_disabled();
    void wait_printer_enabled();

    bool is_eeprom_mmu_enabled();

    void process_printer_request();
    void process_mmu_request();

    void evaluate_sensors();

    FSensor printer_sensor;

    // all those variables can be accessed from multiple threads
    // all of them are set during critical section, so values are guaranted to be coresponding
    // in case multiple values are needed they should be read during critical section too
    std::atomic<uint32_t> event_lock;    // 0 == unlocked
    std::atomic<uint32_t> autoload_lock; // 0 == unlocked
    std::atomic<fsensor_t> state_of_mmu_sensor = fsensor_t::NotInitialized;
    std::atomic<fsensor_t> state_of_printer_sensor = fsensor_t::NotInitialized;
    std::atomic<init_status_t> init_status = init_status_t::NotReady;
    std::atomic<bool> m600_sent = false;
    std::atomic<bool> autoload_sent = false;
    std::atomic<bool> has_mmu = false;

    std::atomic<cmd_t> request_mmu = cmd_t::null;
    std::atomic<cmd_t> request_printer = cmd_t::null;

    std::atomic<bool> mmu_nok = false;
    std::atomic<bool> printer_nok = false;

    std::atomic<inject_t> send_event_on = inject_t::on_edge;

    friend FSensor &GetPrinterFSensor();
};

// singleton
FilamentSensors &FSensors_instance();
