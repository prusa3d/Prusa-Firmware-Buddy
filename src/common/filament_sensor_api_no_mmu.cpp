/**
 * @file filament_sensor_api_no_mmu.cpp
 * @brief this file contains code MMU for filament sensor api without MMU support
 */

#include "filament_sensor_api.hpp"

bool FilamentSensors::is_eeprom_mmu_enabled() {
    return false;
}

void FilamentSensors::process_mmu_request() {
}

void FilamentSensors::mmu_loop() {
    has_mmu = false;
    state_of_mmu_sensor = fsensor_t::Disabled;
    request_mmu = cmd_t::null;
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
    return GetPrinter() == fsensor_t::HasFilament;
}
