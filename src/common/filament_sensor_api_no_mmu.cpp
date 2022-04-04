/**
 * @file filament_sensor_api_no_mmu.cpp
 * @brief this file contains code MMU for filament sensor api without MMU support
 */

#include "filament_sensor_api.hpp"
#include "freertos_mutex.hpp"
#include <mutex>

bool FilamentSensors::is_eeprom_mmu_enabled() {
    return false;
}

void FilamentSensors::process_mmu_request() {
}

// this function should actually be in shared part of code
// and only evaluation of state_of_mmu_sensor should be splitted
// but since we don't have LTO I have made more efficient version
void FilamentSensors::evaluate_sensors() {
    // need locks, to ensure all variables have coresponding values
    // don't actually take the locks yet
    std::unique_lock lock_mmu(mutex_mmu, std::defer_lock);
    std::unique_lock lock_printer(mutex_printer, std::defer_lock);

    // lock both unique_locks without deadlock
    std::lock(lock_mmu, lock_printer);

    has_mmu = false;
    state_of_mmu_sensor = fsensor_t::Disabled;
    request_mmu = cmd_t::null;

    // copy printer sensor state while we are in critical section
    state_of_printer_sensor = printer_sensor.Get();

    if (request_printer != cmd_t::null) {
        init_status = init_status_t::NotReady;
    } else {
        const bool printer_ok = FilamentSensors::IsWorking(state_of_printer_sensor) || state_of_printer_sensor == fsensor_t::Disabled;
        init_status = printer_ok ? init_status_t::Ok : init_status_t::PrinterNotInitialized;
    }
}

void FilamentSensors::mmu_disable() {
}

void FilamentSensors::mmu_enable() {
}

void FilamentSensors::DisableMMU() {
}

FilamentSensors::mmu_enable_result_t FilamentSensors::EnableMMU() {
    return mmu_enable_result_t::error_mmu_not_supported;
}

fsensor_t FilamentSensors::Get() {
    return get();
}

fsensor_t FilamentSensors::get() const {
    return state_of_printer_sensor;
}

bool FilamentSensors::CanStartPrint() {
    return GetPrinter() == fsensor_t::HasFilament || GetPrinter() == fsensor_t::Disabled;
}
