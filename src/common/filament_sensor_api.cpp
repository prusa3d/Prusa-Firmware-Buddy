/**
 * @file filament_sensor_api.cpp
 * @brief this file contains shared code for both MMU and non MMU versions.
 * there are two other files filament_sensor_api_no_mmu, filament_sensor_api_mmu
 * I would normally use inheritance, but last time i did that it was rewritten, so I am using this approach now.
 */

#include "filament_sensor_api.hpp"
#include "filament_sensor_selftest_api.hpp"
#include "print_processor.hpp"
#include "rtos_api.hpp"
#include "freertos_mutex.hpp"
#include "bsod.h"
#include <mutex>

using namespace MMU2;

FilamentSensors::FilamentSensors(FreeRTOS_Mutex &mmu, FreeRTOS_Mutex &printer)
    : mutex_mmu(mmu)
    , mutex_printer(printer) {
    if (is_eeprom_mmu_enabled()) {
        request_mmu = cmd_t::on;
    }
}

char FilamentSensors::GetM600_send_on() const {
    switch (send_event_on) {
    case inject_t::on_edge:
        return 'e';
    case inject_t::on_level:
        return 'l';
    case inject_t::never:
        return 'n';
    }
    return 'x';
}

// Store request_printer off
void FilamentSensors::Disable() {
    DisableMMU(); // MMU requires enabled filament sensor to work
    const std::lock_guard lock(mutex_printer);
    request_printer = cmd_t::off;
}

// Store request_printer off
void FilamentSensors::Enable() {
    const std::lock_guard lock(mutex_printer);
    request_printer = cmd_t::on;
}

// process printer request stored by Enable/Disable
void FilamentSensors::process_printer_request() {
    switch (request_printer) {
    case cmd_t::on:
        printer_sensor.Enable();
        request_printer = cmd_t::processing;
        break;
    case cmd_t::off:
        // TODO this should turn of MMU too
        // request_mmu = cmd_t::off;
        printer_sensor.Disable();
        request_printer = cmd_t::processing;
        break;
    case cmd_t::processing:
    case cmd_t::null:
        break;
    }
}

void FilamentSensors::Cycle() {
    process_mmu_request();
    process_printer_request();

    auto opt_event = printer_sensor.Cycle();
    {
        //need lock to avoid lost of command if it set just after if evaluation but before cmd_t::null assignment
        const std::lock_guard lock(mutex_printer);
        if (request_printer == cmd_t::processing) {
            request_printer = cmd_t::null; // cycle ended clear command, it is atomic so it will not be reordered
        }
    }

    evaluate_sensors();

    std::optional<bool> opt_event_m600 = std::nullopt;
    std::optional<bool> opt_event_autoload = std::nullopt;
    if (opt_event) {
        if (PrintProcessor::IsPrinting()) {
            opt_event_m600 = evaluateM600(*opt_event);
        } else {
            opt_event_autoload = evaluateAutoload(*opt_event);
        }
    }

    if (isEvLocked())
        return;

    // gcode is injected outside critical section, so critical section is as short as possible
    // also injection of GCode inside critical section might not work
    // TODO M600_sent and Autoload_sent should be mutally exclusive
    // for now M600 just has higher prior thanks to "else if"
    if (*opt_event_m600) {
        m600_sent = true;
        PrintProcessor::InjectGcode("M600"); //change filament
    } else if (*opt_event_autoload && !has_mmu && !isAutoloadLocked()) {
        autoload_sent = true;
        PrintProcessor::InjectGcode("M1701"); //autoload with return option
    }
}

// this methot is currently called outside FilamentSensors::Cycle critical section, so the critical section is shorter
// trying to trigger runout at exact moment when print ended could break something
// also if another M600 happens during clear of M600_sent flag, it could be discarded, this is not a problem, because it could happen only due a bug
// if it happens move it inside FilamentSensors::Cycle critical section
bool FilamentSensors::evaluateM600(FSensor::event ev) const {
    if (((send_event_on == inject_t::on_edge && ev == FSensor::event::EdgeFilamentRemoved)
            || (send_event_on == inject_t::on_level && ev == FSensor::event::NoFilament))
        && !m600_sent) {

        return true;
    }
    return false;
}

// this methot is currently called outside FilamentSensors::Cycle critical section, so the critical section is shorter
// trying to trigger autoload at exact moment when print starts could break something
// also if another autoload happens during clear of Autoload_sent flag, it could be discarded, this is not a problem, because it could happen only due a bug
// if it happens move it inside FilamentSensors::Cycle critical section
bool FilamentSensors::evaluateAutoload(FSensor::event ev) const {
    if (((send_event_on == inject_t::on_edge && ev == FSensor::event::EdgeFilamentInserted)
            || (send_event_on == inject_t::on_level && ev == FSensor::event::HasFilament))
        && !autoload_sent
        && PrintProcessor::IsAutoloadEnabled()) {

        return true;
    }
    return false;
}

uint32_t FilamentSensors::DecEvLock() {
    CriticalSection C; // TODO use counting semaphore to avoid critical section
    if (event_lock > 0) {
        --event_lock;
    } else {
        bsod("Filament sensor event lock out of range");
    }
    return event_lock;
}

uint32_t FilamentSensors::DecAutoloadLock() {
    CriticalSection C; // TODO use counting semaphore to avoid critical section
    if (autoload_lock > 0) {
        --autoload_lock;
    } else {
        bsod("Autoload event lock out of range");
    }
    return autoload_lock;
}

uint32_t FilamentSensors::IncAutoloadLock() {
    return ++autoload_lock;
}
uint32_t FilamentSensors::IncEvLock() {
    return ++event_lock;
}

FilamentSensors::inject_t FilamentSensors::getM600_send_on_and_disable() {
    CriticalSection C;
    inject_t ret = send_event_on;
    send_event_on = inject_t::never;
    return ret;
}
void FilamentSensors::restore_send_M600_on(inject_t send_event_on_) {
    send_event_on = send_event_on_;
}

void FilamentSensors::printer_disable() {
    printer_sensor.Disable();
    wait_printer_disabled();
}

void FilamentSensors::printer_enable() {
    printer_sensor.Enable();
    wait_printer_enabled();
}

void FilamentSensors::wait_printer_disabled() {
    while (state_of_printer_sensor != fsensor_t::Disabled) {
        Rtos::Delay(0); // switch to other threads
    }
}

void FilamentSensors::wait_printer_enabled() {
    while (state_of_printer_sensor == fsensor_t::NotInitialized || state_of_printer_sensor == fsensor_t::Disabled) {
        Rtos::Delay(0); // switch to other threads
    }
}

/*****************************************************************************/
// section with locks
// Do not nest calls of methods with same mutex !!!
// Do not call from filament sensor thread

/**
 * @brief encode printer sensor state to MMU enum
 * TODO distingish between at fsensor and in nozzle
 * currently only AT_FSENSOR returned
 * @return MMU2::FilamentState
 */
FilamentState FilamentSensors::WhereIsFilament() {
    const std::lock_guard lock(mutex_printer);
    switch (state_of_printer_sensor) {
    case fsensor_t::HasFilament:
        return FilamentState::AT_FSENSOR;
    case fsensor_t::NoFilament:
        return FilamentState::NOT_PRESENT;
    case fsensor_t::NotInitialized:
    case fsensor_t::NotCalibrated:
    case fsensor_t::NotConnected:
    case fsensor_t::Disabled:
        break;
    }
    return FilamentState::UNAVAILABLE;
}

// this method should not be accessed if we don't have MMU
// if it is it will do unnecessary lock and return false
// I prefer to have single method for both variants
bool FilamentSensors::HasMMU() {
    const std::lock_guard lock(mutex_mmu);
    return has_mmu;
}

fsensor_t FilamentSensors::GetPrinter() {
    const std::lock_guard lock(mutex_printer);
    return state_of_printer_sensor;
}

// this method should not be accessed if we don't have MMU
// if it is it will do unnecessary lock and return disabled
// I prefer to have single method for both variants
fsensor_t FilamentSensors::GetMMU() {
    const std::lock_guard lock(mutex_mmu);
    return state_of_mmu_sensor;
}
// end of section with locks
/*****************************************************************************/

// singleton
FilamentSensors &FSensors_instance() {
    static FreeRTOS_Mutex m[2];
    static FilamentSensors ret(m[0], m[1]);
    return ret;
}

// friend function for selftest
FSensor &GetPrinterFSensor() {
    return FSensors_instance().printer_sensor;
}
