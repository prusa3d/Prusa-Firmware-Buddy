/**
 * @file filament_sensors_handler.cpp
 * @brief this file contains shared code for both MMU and non MMU versions.
 * there are two other files filament_sensors_handler_no_mmu, filament_sensors_handler_mmu
 * I would normally use inheritance, but last time i did that it was rewritten, so I am using this approach now.
 */

#include "filament_sensors_handler.hpp"
#include "print_processor.hpp"
#include "rtos_api.hpp"
#include <common/freertos_mutex.hpp>
#include "bsod.h"
#include <mutex>
#include <log.h>
#include <option/has_selftest_snake.h>
#include <option/has_mmu2.h>
#include <option/has_human_interactions.h>
#if HAS_SELFTEST_SNAKE()
    #include <ScreenHandler.hpp>
    #include "screen_menu_selftest_snake.hpp"
#endif

LOG_COMPONENT_DEF(FSensor, LOG_SEVERITY_INFO);

using namespace MMU2;

FilamentSensors::FilamentSensors() {
    reconfigure_sensors_if_needed(true);

    // Request that the fsensors get properly configured on startup
    enable_state_update_pending = true;
}

freertos::Mutex &FilamentSensors::GetExtruderMutex() const {
    static freertos::Mutex ret;
    return ret;
}

void FilamentSensors::set_enabled_global(bool set) {
    if (config_store().fsensor_enabled.set(set)) {
        request_enable_state_update();
    }
}

void FilamentSensors::request_enable_state_update() {
#if HAS_MMU2()
    // MMU requires enabled filament sensor to work, it makes sense for XL to behave the same
    if (!config_store().fsensor_enabled.get()) {
        marlin_client::gcode("M709 S0");
    }
#endif

    enable_state_update_pending = true;
}

void FilamentSensors::for_all_sensors(const std::function<void(IFSensor &)> &f) {
    HOTEND_LOOP() {
        if (IFSensor *s = GetExtruderFSensor(e)) {
            f(*s);
        }
        if (IFSensor *s = GetSideFSensor(e)) {
            f(*s);
        }
    }
}

void FilamentSensors::Cycle() {
    if (enable_state_update_pending) {
        process_enable_state_update();
    }

    // Run cycle to evaluate state of all sensors (even those not active)
    for_all_sensors([](IFSensor &s) {
        if (s.is_enabled()) {
            s.cycle();
        }

        s.record_state();
        s.check_for_events();
    });

    {
        // Usage of the mutex needs reevaluation - I don't really understand when it should be used and when not
        std::unique_lock lock_printer(GetExtruderMutex());

        reconfigure_sensors_if_needed(false);

        for (uint8_t i = 0; i < logical_filament_sensor_count; i++) {
            IFSensor *fs = logical_sensors_.array[i];
            logical_sensor_states_.array[i] = fs ? fs->get_state() : FilamentSensorState::Disabled;
        }
    }

    process_events();
}

void FilamentSensors::process_events() {
    if (isEvLocked()) {
        return;
    }

    const auto check_runout = [&](LogicalFilamentSensor sensor) {
        const auto event = logical_sensors_[sensor]->last_event();
        if (m600_sent || event != IFSensor::Event::filament_removed) {
            return false;
        }

        m600_sent = true;

        if constexpr (option::has_human_interactions) {
            PrintProcessor::InjectGcode("M600 A"); // change filament
        } else {
            PrintProcessor::InjectGcode("M25 U"); // pause and unload filament
        }

        log_info(FSensor, "Injected runout");
        return true;
    };

    const auto check_autoload = [&]() {
        const auto event = logical_sensors_[LogicalFilamentSensor::autoload]->last_event();

        if (
            event != IFSensor::Event::filament_inserted
            || has_mmu
            || autoload_sent
            || isAutoloadLocked()
            || !PrintProcessor::IsAutoloadEnabled() //
#if HAS_SELFTEST_SNAKE()
            // We're accessing screens from the filamentsensors thread here. This looks quite unsafe.
            || Screens::Access()->IsScreenOnStack<ScreenMenuSTSWizard>()
            || Screens::Access()->IsScreenOnStack<ScreenMenuSTSCalibrations>()
#endif /*PRINTER_IS_PRUSA_XL*/
        ) {
            return false;
        }

        autoload_sent = true;
        PrintProcessor::InjectGcode("M1701 Z40"); // autoload with return option and minimal Z value of 40mm
        log_info(FSensor, "Injected autoload");

        return true;
    };

    if (PrintProcessor::IsPrinting()) {
        if (check_runout(LogicalFilamentSensor::primary_runout)) {
            return;
        }

        // With an MMU, don't check for runout on the secondary sensor
        if (!has_mmu && check_runout(LogicalFilamentSensor::secondary_runout)) {
            return;
        }

    } else {
        if (check_autoload()) {
            return;
        }
    }
}

void FilamentSensors::process_enable_state_update() {
    enable_state_update_processing = true;
    enable_state_update_pending = false;

    const bool enabled = config_store().fsensor_enabled.get();

    for_all_sensors([&](IFSensor &s) {
        s.set_enabled(enabled);
    });

    enable_state_update_processing = false;
}

void FilamentSensors::DecEvLock() {
    if ((event_lock--) == 0) {
        bsod("Filament sensor event underflow");
    }
}
void FilamentSensors::IncEvLock() {
    if ((event_lock++) == std::numeric_limits<decltype(autoload_lock)::value_type>::max()) {
        bsod("Filament sensor event lock overflow");
    }
}

void FilamentSensors::DecAutoloadLock() {
    if ((autoload_lock--) == 0) {
        bsod("Autoload event lock underflow");
    }
}
void FilamentSensors::IncAutoloadLock() {
    if ((autoload_lock++) == std::numeric_limits<decltype(autoload_lock)::value_type>::max()) {
        bsod("Autoload sensor event lock overflow");
    }
}

bool FilamentSensors::MMUReadyToPrint() {
    std::unique_lock lock_printer(GetExtruderMutex());

    // filament has to be unloaded from primary tool for MMU print
    return logical_sensor_states_[LogicalFilamentSensor::primary_runout] == FilamentSensorState::NoFilament;
}

bool FilamentSensors::ToolHasFilament(uint8_t tool_nr) {
    std::unique_lock lock_printer(GetExtruderMutex());

    FilamentSensorState extruder_state = GetExtruderFSensor(tool_nr) ? GetExtruderFSensor(tool_nr)->get_state() : FilamentSensorState::Disabled;
    FilamentSensorState side_state = GetSideFSensor(tool_nr) ? GetSideFSensor(tool_nr)->get_state() : FilamentSensorState::Disabled;

    return (extruder_state == FilamentSensorState::HasFilament || extruder_state == FilamentSensorState::Disabled) && (side_state == FilamentSensorState::HasFilament || side_state == FilamentSensorState::Disabled);
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
    switch (logical_sensor_states_[LogicalFilamentSensor::secondary_runout]) {
    case FilamentSensorState::HasFilament:
        return FilamentState::AT_FSENSOR;
    case FilamentSensorState::NoFilament:
        return FilamentState::NOT_PRESENT;
    case FilamentSensorState::NotInitialized:
    case FilamentSensorState::NotCalibrated:
    case FilamentSensorState::NotConnected:
    case FilamentSensorState::Disabled:
    case FilamentSensorState::_cnt:
        break;
    }
    return FilamentState::UNAVAILABLE;
}

// this method should not be accessed if we don't have MMU
// if it is it will do unnecessary lock and return false
// I prefer to have single method for both variants
bool FilamentSensors::HasMMU() {
    const std::lock_guard lock(GetExtruderMutex());
    return has_mmu;
}

// end of section with locks
/*****************************************************************************/

// Meyer's singleton
FilamentSensors &FSensors_instance() {
    static FilamentSensors ret;
    return ret;
}
