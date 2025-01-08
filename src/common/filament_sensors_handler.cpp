/**
 * @file filament_sensors_handler.cpp
 * @brief this file contains shared code for both MMU and non MMU versions.
 * there are two other files filament_sensors_handler_no_mmu, filament_sensors_handler_mmu
 * I would normally use inheritance, but last time i did that it was rewritten, so I am using this approach now.
 */

#include "filament_sensors_handler.hpp"
#include "bsod.h"
#include <tasks.hpp>
#include "window_msgbox.hpp"
#include <logging/log.hpp>
#include <option/has_selftest.h>
#include <option/has_mmu2.h>
#include <option/has_toolchanger.h>
#include <stdio.h>

#if HAS_SELFTEST()
    #include <ScreenHandler.hpp>
    #include "screen_menu_selftest_snake.hpp"
#endif

#if HAS_MMU2()
    #include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#endif

LOG_COMPONENT_DEF(FSensor, logging::Severity::info);

using namespace MMU2;

FilamentSensors::FilamentSensors() {
    reconfigure_sensors_if_needed(true);

    // Request that the fsensors get properly configured on startup
    enable_state_update_pending = true;
}

void FilamentSensors::set_enabled_global(bool set) {
    if (config_store().fsensor_enabled.get() == set) {
        return;
    }

    config_store().fsensor_enabled.set(set);
    request_enable_state_update();
}

void FilamentSensors::request_enable_state_update([[maybe_unused]] bool check_fs) {
#if HAS_MMU2()
    // MMU requires enabled filament sensor to work, it makes sense for XL to behave the same
    if (check_fs && config_store().mmu2_enabled.get() && !config_store().fsensor_enabled.get()) {
        marlin_client::gcode("M709 S0");
    }
#endif

    enable_state_update_pending = true;
}

bool FilamentSensors::gui_wait_for_init_with_msg() {
    enum : uint8_t {
        f_extruder = 1,
        f_side = 2,
    };

    const auto any_fsensor_in_state
        = [&](FilamentSensorState state) {
              uint8_t result = 0;
              for_all_sensors([&](IFSensor &s, [[maybe_unused]] uint8_t index, bool is_side) {
                  if (s.get_state() == state) {
                      result |= is_side ? f_side : f_extruder;
                  }
              });
              return result;
          };

    // wait until it is initialized
    // no guiloop here !!! - it could cause show of unwanted error message
    while (is_enable_state_update_processing() || any_fsensor_in_state(FilamentSensorState::NotInitialized)) {
        osDelay(0);
    }

    if ([[maybe_unused]] auto ncf = any_fsensor_in_state(FilamentSensorState::NotConnected)) {
        if (ncf & f_extruder) {
            MsgBoxError(_("Filament sensor not connected, check wiring."), Responses_Ok);
            return false;

        } else {
            // Only side sensors are not connected, not that tragic, show message but keep on going
            MsgBoxWarning(_("Side filament sensor not connected, check wiring."), Responses_Ok);
        }
    }

    if (any_fsensor_in_state(FilamentSensorState::NotCalibrated)) {
        MsgBoxWarning(_("Filament sensor not ready: perform calibration first."), Responses_Ok);
        return false;
    }

    return true;
}

void FilamentSensors::for_all_sensors(const stdext::inplace_function<void(IFSensor &sensor, uint8_t index, bool is_side)> &f) {
    HOTEND_LOOP() {
        if (IFSensor *s = GetExtruderFSensor(e)) {
            f(*s, e, false);
        }
        if (IFSensor *s = GetSideFSensor(e)) {
            f(*s, e, true);
        }
    }
}

void FilamentSensors::task_init() {
    marlin_client::init();
}

void FilamentSensors::task_cycle() {
    marlin_client::loop();

    static bool old_state = false;
    const bool new_state = marlin_vars().get_fsm_states().is_active(ClientFSM::Load_unload);

    if (old_state && !new_state) {
        FSensors_instance().DecEvLock(); // ClientFSM::Load_unload destroy
    }
    if (!old_state && new_state) {
        FSensors_instance().IncEvLock(); // ClientFSM::Load_unload create
    }

    old_state = new_state;

    // Reconfigure logical sensors
    reconfigure_sensors_if_needed(false);

    // Update states of filament sensors
    if (enable_state_update_pending) {
        process_enable_state_update();
    }

    // Run cycle to evaluate state of all sensors (even those not active)
    for_all_sensors([](IFSensor &s, uint8_t, bool) {
        if (s.is_enabled()) {
            s.cycle();
        }

        s.record_state();
        s.check_for_events();
    });

    // Update logical sensors states
    for (uint8_t i = 0; i < logical_filament_sensor_count; i++) {
        IFSensor *fs = logical_sensors_.array[i];
        logical_sensor_states_.array[i] = fs ? fs->get_state() : FilamentSensorState::Disabled;
    }

    process_events();
}

void FilamentSensors::reconfigure_sensors_if_needed(bool force) {
    const uint8_t new_tool_index =
#if HAS_TOOLCHANGER()
        prusa_toolchanger.get_active_tool_nr();
#else
        0;
#endif

    const bool new_has_mmu =
#if HAS_MMU2()
        mmu2.State() != xState::Stopped;
#else
        false;
#endif

    if (!force && new_tool_index == tool_index && new_has_mmu == has_mmu) {
        return;
    }

    tool_index = new_tool_index;
    has_mmu = new_has_mmu;

    using LFS = LogicalFilamentSensor;
    auto &ls = logical_sensors_;

    const auto extruder_fs = GetExtruderFSensor(tool_index);
    const auto side_fs = GetSideFSensor(tool_index);

    ls[LFS::extruder] = extruder_fs;
    ls[LFS::side] = side_fs;
    ls[LFS::primary_runout] = side_fs ?: extruder_fs;
    ls[LFS::secondary_runout] = side_fs ? extruder_fs : nullptr;
#if PRINTER_IS_PRUSA_iX()
    /**  iX can behave a little bit differently when autoloading thanks to it being outside of user's reach. The head will move during autohoming and could cause harm to person having hands within head's space.
        If the autoload would be triggered by extruder fs it could mean that user is trying to insert filament while manipulating with head itself, an action that could cause harm.
        This can change in the future but needs some thought on printer's behaviour in such case (e.g. filament is already in extruder, there is no need for parking movement) */
    ls[LFS::autoload] = side_fs ?: nullptr;
#else
    ls[LFS::autoload] = has_mmu ? nullptr : extruder_fs;
#endif
}

void FilamentSensors::process_events() {
    if (isEvLocked()) {
        return;
    }

    const auto check_runout = [&](LogicalFilamentSensor s) {
        const auto event = sensor(s)->last_event();
        if (m600_sent || event != IFSensor::Event::filament_removed) {
            return false;
        }

        m600_sent = true;

        marlin_client::inject("M600 A"); // change filament

        log_info(FSensor, "Injected runout");
        return true;
    };

    const auto check_autoload = [&]() {
        const auto event = sensor(LogicalFilamentSensor::autoload)->last_event();

        if (event != IFSensor::Event::filament_inserted
            || has_mmu
            || autoload_sent
            || isAutoloadLocked()
            || !marlin_vars().fs_autoload_enabled
#if HAS_SELFTEST()
            // We're accessing screens from the filamentsensors thread here. This looks quite unsafe.
            || Screens::Access()->IsScreenOnStack<ScreenMenuSTSWizard>()
            || Screens::Access()->IsScreenOnStack<ScreenMenuSTSCalibrations>()
#endif
        ) {
            return false;
        }

        autoload_sent = true;
        static char buffer[sizeof("M1701 ZXXXXX")];
        snprintf(buffer, sizeof(buffer), "M1701 Z%.2f", static_cast<double>(Z_AXIS_LOAD_POS));
        // autoload with return option and minimal Z value of 40mm
        // This is a hack, but there is currently no nice way to do snprintf at compile  time
        // We're always writing the same string to the buffer, so there is no race condition
        marlin_client::inject(ConstexprString::from_str_unsafe(buffer));
        log_info(FSensor, "Injected autoload");

        return true;
    };

    if (marlin_client::is_printing()) {
        if (check_runout(LogicalFilamentSensor::primary_runout)) {
            return;
        }

        // With an MMU, don't check for runout on the secondary sensor
        if (!has_mmu && check_runout(LogicalFilamentSensor::secondary_runout)) {
            return;
        }
    } else {
        // During MMU standard operation, there is no filament loaded to the nozzle when not printing.
        // So it's not a good idea to reset what filament types we have stored.
        if (!has_mmu && sensor(LogicalFilamentSensor::extruder)->get_state() == FilamentSensorState::NoFilament) {
            config_store().set_filament_type(tool_index, FilamentType::none);
        }

        if (check_autoload()) {
            return;
        }
    }
}

void FilamentSensors::process_enable_state_update() {
    enable_state_update_processing = true;
    enable_state_update_pending = false;

    const bool global_enable = config_store().fsensor_enabled.get();
    const uint8_t extruder_enable_bits = config_store().fsensor_extruder_enabled_bits.get();
    const uint8_t side_enable_bits = config_store().fsensor_side_enabled_bits.get();

    HOTEND_LOOP() {
        if (IFSensor *s = GetExtruderFSensor(e)) {
            s->set_enabled(global_enable && (extruder_enable_bits & (1 << e)));
        }
        if (IFSensor *s = GetSideFSensor(e)) {
            s->set_enabled(global_enable && (side_enable_bits & (1 << e)));
        }
    }

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
    // filament has to be unloaded from primary tool for MMU print
    return logical_sensor_states_[LogicalFilamentSensor::primary_runout] == FilamentSensorState::NoFilament;
}

bool FilamentSensors::ToolHasFilament(uint8_t tool_nr) {
    FilamentSensorState extruder_state = GetExtruderFSensor(tool_nr) ? GetExtruderFSensor(tool_nr)->get_state() : FilamentSensorState::Disabled;
    FilamentSensorState side_state = GetSideFSensor(tool_nr) ? GetSideFSensor(tool_nr)->get_state() : FilamentSensorState::Disabled;

    return (extruder_state == FilamentSensorState::HasFilament || extruder_state == FilamentSensorState::Disabled) && (side_state == FilamentSensorState::HasFilament || side_state == FilamentSensorState::Disabled || side_state == FilamentSensorState::NotConnected);
}

/**
 * @brief encode printer sensor state to MMU enum
 * TODO distinguish between at fsensor and in nozzle
 * currently only AT_FSENSOR returned
 * @return MMU2::FilamentState
 */
FilamentState FilamentSensors::WhereIsFilament() {
    switch (logical_sensor_states_[LogicalFilamentSensor::secondary_runout]) {

    case FilamentSensorState::HasFilament:
        return FilamentState::AT_FSENSOR;

    case FilamentSensorState::NoFilament:
        return FilamentState::NOT_PRESENT;

    default:
        return FilamentState::UNAVAILABLE;
    }
}

// Meyer's singleton
FilamentSensors &FSensors_instance() {
    static FilamentSensors ret;
    return ret;
}
