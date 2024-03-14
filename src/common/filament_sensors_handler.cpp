/**
 * @file filament_sensors_handler.cpp
 * @brief this file contains shared code for both MMU and non MMU versions.
 * there are two other files filament_sensors_handler_no_mmu, filament_sensors_handler_mmu
 * I would normally use inheritance, but last time i did that it was rewritten, so I am using this approach now.
 */

#include "filament_sensors_handler.hpp"
#include "print_processor.hpp"
#include "rtos_api.hpp"
#include "freertos_mutex.hpp"
#include "bsod.h"
#include <mutex>
#include <log.h>
#include "fsensor_eeprom.hpp"
#include <option/has_selftest_snake.h>
#include <option/has_mmu2.h>
#if HAS_SELFTEST_SNAKE()
    #include <ScreenHandler.hpp>
    #include "screen_menu_selftest_snake.hpp"
#endif

LOG_COMPONENT_DEF(FSensor, LOG_SEVERITY_INFO);

using namespace MMU2;

FilamentSensors::FilamentSensors() {

    SetToolIndex(); // other config depends on it, do it first

    if (has_mmu2_enabled()) {
        request_side = filament_sensor::cmd_t::on;
    }

    // Set logical sensors
    // MK4 can be reconfigured (connecting MMU)
    // MINI can not
    // XL reconfigures on tool change
    configure_sensors();
}

FreeRTOS_Mutex &FilamentSensors::GetSideMutex() {
    static FreeRTOS_Mutex ret;
    return ret;
}
FreeRTOS_Mutex &FilamentSensors::GetExtruderMutex() {
    static FreeRTOS_Mutex ret;
    return ret;
}

// there is only one init_status
// active extruder is stored in FilamentSensors
// changing it must change init_status too
filament_sensor::init_status_t FilamentSensors::get_active_init_status() const {
    return init_status;
}

// Store request_printer off
void FilamentSensors::Disable() {
    DisableSideSensor(); // MMU requires enabled filament sensor to work, it makes sense for XL to behave the same
    const std::lock_guard lock(GetExtruderMutex());
    request_printer = filament_sensor::cmd_t::off;
}

// Store request_printer on
void FilamentSensors::Enable() {
    const std::lock_guard lock(GetExtruderMutex());
    request_printer = filament_sensor::cmd_t::on;
}

bool FilamentSensors::is_enabled() const {
    return FSensorEEPROM::Get();
}

auto FilamentSensors::get_enable_result() -> EnableResult {
    filament_sensor::cmd_t request = request_printer.load();
    if (request == filament_sensor::cmd_t::on || request == filament_sensor::cmd_t::processing) {
        return EnableResult::in_progress;
    }

    // Check state of all sensors
    HOTEND_LOOP() {
        if (IFSensor *s = GetExtruderFSensor(e); s) {
            fsensor_t state = s->Get();
            switch (state) {
            case fsensor_t::NotInitialized:
                return EnableResult::in_progress;
            case fsensor_t::NotConnected:
                return EnableResult::not_connected;
            case fsensor_t::NotCalibrated:
                return EnableResult::not_calibrated;
            default:
                break;
            }
        }
        if (IFSensor *s = GetSideFSensor(e); s) {
            fsensor_t state = s->Get();
            switch (state) {
            case fsensor_t::NotInitialized:
                return EnableResult::in_progress;
            case fsensor_t::NotConnected:
                return EnableResult::not_connected;
            case fsensor_t::NotCalibrated:
                return EnableResult::not_calibrated;
            default:
                break;
            }
        }
    }

    return EnableResult::ok;
}

// process printer request stored by Enable/Disable
void FilamentSensors::process_printer_request() {
    switch (request_printer) {
    case filament_sensor::cmd_t::on:

        HOTEND_LOOP() {
            if (IFSensor *s = GetExtruderFSensor(e); s) {
                s->Enable();
            }
            if (IFSensor *s = GetSideFSensor(e); s) {
                s->Enable();
            }
        }
        request_printer = filament_sensor::cmd_t::processing;
        break;
    case filament_sensor::cmd_t::off:
        HOTEND_LOOP() {
            if (IFSensor *s = GetExtruderFSensor(e); s) {
                s->Disable();
            }
            if (IFSensor *s = GetSideFSensor(e); s) {
                s->Disable();
            }
        }

        request_printer = filament_sensor::cmd_t::processing;
        break;
    case filament_sensor::cmd_t::processing:
    case filament_sensor::cmd_t::null:
        break;
    }
}

void FilamentSensors::all_sensors_initialized() {
    // Need lock to avoid loss of command if it is set just after evaluation but before assignment
    const std::lock_guard lock(GetExtruderMutex());
    if (request_printer == filament_sensor::cmd_t::processing) {
        request_printer = filament_sensor::cmd_t::null; // cycle ended clear command, it is atomic so it will not be reordered
    }
}
bool FilamentSensors::run_sensors_cycle() {
    bool any_not_intitialized = false;
    HOTEND_LOOP() {
        if (IFSensor *s = GetExtruderFSensor(e); s != nullptr) {
            s->Cycle();
            if (s->Get() == fsensor_t::NotInitialized) {
                any_not_intitialized = true;
            }
        }
        if (IFSensor *s = GetSideFSensor(e); s != nullptr) {
            s->Cycle();
            if (s->Get() == fsensor_t::NotInitialized) {
                any_not_intitialized = true;
            }
        }
    }
    return any_not_intitialized;
}

filament_sensor::Events FilamentSensors::evaluate_logical_sensors_events() {
    auto arr_logical = logical_sensors.get_array();

    // Get events for each logical sensor.
    filament_sensor::Events ret;
    for (size_t logi = 0; logi < arr_logical.size(); ++logi) {
        if (arr_logical[logi]) { // check if current sensor is linked
            ret.get(logi) = arr_logical[logi]->GenerateEvent();

            // Since sensor can repeat inside arr_logical, update every consecutive logical sensor and set it to null so GenerateEvent is not called twice
            for (size_t next_logi = logi + 1; next_logi < arr_logical.size(); ++next_logi) {
                if (arr_logical[logi] == arr_logical[next_logi]) {
                    ret.get(next_logi) = ret.get(logi);
                    arr_logical[next_logi] = nullptr;
                }
            }
        }
    }

    return ret;
}

void FilamentSensors::Cycle() {
    process_side_request();
    process_printer_request();

    // run cycle to evaluate state of all sensors (even those not active)
    bool any_not_intitialized = run_sensors_cycle();

    if (!any_not_intitialized) {
        all_sensors_initialized();
    }

    // Evaluate currently used sensors of all sensors
    filament_sensor::Events events = FilamentSensors::evaluate_logical_sensors_events();

    set_corresponding_variables();

    bool opt_event_m600 { false };
    bool opt_event_autoload { false };

    if (PrintProcessor::IsPrinting()) {
        if (events.primary_runout) {
            opt_event_m600 = evaluateM600(*events.primary_runout);
        }

        // With an MMU, don't check for runout on the secondary sensor
        if (!has_mmu && !opt_event_m600 && events.secondary_runout) {
            opt_event_m600 = evaluateM600(*events.secondary_runout);
        }
    } else {
        if (events.autoload) {
            opt_event_autoload = evaluateAutoload(*events.autoload);
        }
    }

    if (isEvLocked()) {
        return;
    }

    // gcode is injected outside critical section, so critical section is as short as possible
    // also injection of GCode inside critical section might not work
    // TODO M600_sent and Autoload_sent should be mutually exclusive
    // for now M600 just has higher prior thanks to "else if"
    if (opt_event_m600) {
        m600_sent = true;
        PrintProcessor::InjectGcode("M600 A"); // change filament
        log_info(FSensor, "Injected runout");
    } else if (opt_event_autoload && !has_mmu && !isAutoloadLocked()
#if HAS_SELFTEST_SNAKE()
        && !Screens::Access()->IsScreenOnStack<ScreenMenuSTSWizard>()
        && !Screens::Access()->IsScreenOnStack<ScreenMenuSTSCalibrations>()
#endif /*PRINTER_IS_PRUSA_XL*/
    ) {
        autoload_sent = true;
        PrintProcessor::InjectGcode("M1701 Z40"); // autoload with return option and minimal Z value of 40mm
        log_info(FSensor, "Injected autoload");
    }
}

/**
 * @brief method to store corresponding values inside critical section
 */
void FilamentSensors::set_corresponding_variables() {
    // need locks, to ensure all variables have corresponding values
    // don't actually take the locks yet
    std::unique_lock lock_mmu(GetSideMutex(), std::defer_lock);
    std::unique_lock lock_printer(GetExtruderMutex(), std::defer_lock);

    // lock both unique_locks without deadlock
    buddy::lock(lock_mmu, lock_printer);

    reconfigure_sensors_if_needed();

    // copy states of sensors while we are in critical section
    state_of_primary_runout_sensor = logical_sensors.primary_runout ? logical_sensors.primary_runout->Get() : fsensor_t::Disabled;
    state_of_secondary_runout_sensor = logical_sensors.secondary_runout ? logical_sensors.secondary_runout->Get() : fsensor_t::Disabled;
    state_of_autoload_sensor = logical_sensors.autoload ? logical_sensors.autoload->Get() : fsensor_t::Disabled;

    state_of_current_extruder = logical_sensors.current_extruder ? logical_sensors.current_extruder->Get() : fsensor_t::Disabled;
    state_of_current_side = logical_sensors.current_side ? logical_sensors.current_side->Get() : fsensor_t::Disabled;

    if ((request_printer != filament_sensor::cmd_t::null) || (request_side != filament_sensor::cmd_t::null)) {
        init_status = filament_sensor::init_status_t::NotReady;
    } else {
        // if we don't have sensor it is automatically ok, "having sensor" is set by (re)configure_sensors
        const bool side_sensor_ok =
#if HAS_MMU2()
            !has_mmu || // this might be unnecessary TODO try MK4 with MMU without it
#endif
            !logical_sensors.current_side || FilamentSensors::IsWorking(logical_sensors.current_side->Get());
        const bool extruder_sensor_ok = !logical_sensors.current_extruder || FilamentSensors::IsWorking(logical_sensors.current_extruder->Get()) || logical_sensors.current_extruder->Get() == fsensor_t::Disabled;
        const bool side_not_calibrated = (!side_sensor_ok) && logical_sensors.current_side && (logical_sensors.current_side->Get() == fsensor_t::NotCalibrated);
        const bool extruder_not_calibrated = (!extruder_sensor_ok) && logical_sensors.current_extruder && (logical_sensors.current_extruder->Get() == fsensor_t::NotCalibrated);

        // side_not_calibrated, extruder_not_calibrated, side_sensor_ok, extruder_sensor_ok
        // 2^4 = 16 combinations

        // side_not_calibrated, extruder_not_calibrated, side_sensor_ok, extruder_sensor_ok
        // 1, 1, X, X .. 4 combinations
        if (side_not_calibrated && extruder_not_calibrated) {
            init_status = filament_sensor::init_status_t::BothNotCalibrated;
        }

        // side_not_calibrated, extruder_not_calibrated, side_sensor_ok, extruder_sensor_ok
        // 1, 0, X, X .. 4 combinations
        if (side_not_calibrated && !extruder_not_calibrated) {
            init_status = filament_sensor::init_status_t::SideNotCalibrated;
        }

        // side_not_calibrated, extruder_not_calibrated, side_sensor_ok, extruder_sensor_ok
        // 0, 1, X, X .. 4 combinations
        if (!side_not_calibrated && extruder_not_calibrated) {
            init_status = filament_sensor::init_status_t::ExtruderNotCalibrated;
        }

        // side_not_calibrated, extruder_not_calibrated, side_sensor_ok, extruder_sensor_ok
        // 0, 0, 0, 0 .. 1 combination
        if (!side_not_calibrated && !extruder_not_calibrated && !side_sensor_ok && !extruder_sensor_ok) {
            init_status = filament_sensor::init_status_t::BothNotInitialized;
        }

        // side_not_calibrated, extruder_not_calibrated, side_sensor_ok, extruder_sensor_ok
        // 0, 0, 0, 1 .. 1 combination
        if (!side_not_calibrated && !extruder_not_calibrated && !side_sensor_ok && extruder_sensor_ok) {
            init_status = filament_sensor::init_status_t::SideNotInitialized;
        }

        // side_not_calibrated, extruder_not_calibrated, side_sensor_ok, extruder_sensor_ok
        // 0, 0, 1, 0 .. 1 combination
        if (!side_not_calibrated && !extruder_not_calibrated && side_sensor_ok && !extruder_sensor_ok) {
            init_status = filament_sensor::init_status_t::ExtruderNotInitialized;
        }

        // side_not_calibrated, extruder_not_calibrated, side_sensor_ok, extruder_sensor_ok
        // 0, 0, 1, 1 .. 1 combination
        if (!side_not_calibrated && !extruder_not_calibrated && side_sensor_ok && extruder_sensor_ok) {
            init_status = filament_sensor::init_status_t::Ok;
        }
    }
}

FilamentSensors::BothSensors FilamentSensors::GetBothSensors() {
    // need locks, to ensure all variables have corresponding values
    // don't actually take the locks yet
    std::unique_lock lock_mmu(GetSideMutex(), std::defer_lock);
    std::unique_lock lock_printer(GetExtruderMutex(), std::defer_lock);

    // lock both unique_locks without deadlock
    buddy::lock(lock_mmu, lock_printer);

    BothSensors ret = { state_of_current_extruder, state_of_current_side };

    return ret;
}

// this method is currently called outside FilamentSensors::Cycle critical section, so the critical section is shorter
// trying to trigger runout at exact moment when print ended could break something
// also if another M600 happens during clear of M600_sent flag, it could be discarded, this is not a problem, because it could happen only due a bug
// if it happens move it inside FilamentSensors::Cycle critical section
bool FilamentSensors::evaluateM600(FSensor::event ev) const {
    if ((ev == FSensor::event::EdgeFilamentRemoved)
        && !m600_sent) {
        return true;
    }
    return false;
}

// this method is currently called outside FilamentSensors::Cycle critical section, so the critical section is shorter
// trying to trigger autoload at exact moment when print starts could break something
// also if another autoload happens during clear of Autoload_sent flag, it could be discarded, this is not a problem, because it could happen only due a bug
// if it happens move it inside FilamentSensors::Cycle critical section
bool FilamentSensors::evaluateAutoload(FSensor::event ev) const {
    if ((ev == FSensor::event::EdgeFilamentInserted)
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

bool FilamentSensors::MMUReadyToPrint() {
    // don't actually take the locks yet
    std::unique_lock lock_mmu(GetSideMutex(), std::defer_lock);
    std::unique_lock lock_printer(GetExtruderMutex(), std::defer_lock);

    // lock both unique_locks without deadlock
    buddy::lock(lock_mmu, lock_printer);

    // filament has to be unloaded from primary tool for MMU print
    return state_of_primary_runout_sensor == fsensor_t::NoFilament;
}

bool FilamentSensors::ToolHasFilament(uint8_t tool_nr) {
    // don't actually take the locks yet
    std::unique_lock lock_side(GetSideMutex(), std::defer_lock);
    std::unique_lock lock_printer(GetExtruderMutex(), std::defer_lock);

    // lock both unique_locks without deadlock
    buddy::lock(lock_side, lock_printer);

    fsensor_t extruder_state = GetExtruderFSensor(tool_nr) ? GetExtruderFSensor(tool_nr)->Get() : fsensor_t::Disabled;
    fsensor_t side_state = GetSideFSensor(tool_nr) ? GetSideFSensor(tool_nr)->Get() : fsensor_t::Disabled;

    return (extruder_state == fsensor_t::HasFilament || extruder_state == fsensor_t::Disabled) && (side_state == fsensor_t::HasFilament || side_state == fsensor_t::Disabled);
}

IFSensor *get_active_printer_sensor() {
    return GetExtruderFSensor(FSensors_instance().tool_index);
}

IFSensor *get_active_side_sensor() {
    return GetSideFSensor(FSensors_instance().tool_index);
}

/*****************************************************************************/
// section with locks
// Do not nest calls of methods with same mutex !!!
// Do not call from filament sensor thread

/**
 * @brief encode printer sensor state to MMU enum
 * TODO distinguish between at fsensor and in nozzle
 * currently only AT_FSENSOR returned
 * @return MMU2::FilamentState
 */
FilamentState FilamentSensors::WhereIsFilament() {
    const std::lock_guard lock(GetExtruderMutex());
    switch (state_of_secondary_runout_sensor) {
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
    const std::lock_guard lock(GetSideMutex());
    return has_mmu;
}

// end of section with locks
/*****************************************************************************/

// Meyer's singleton
FilamentSensors &FSensors_instance() {
    static FilamentSensors ret;
    return ret;
}
