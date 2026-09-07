/**
 * @file filament_sensors_handler.cpp
 * @brief this file contains shared code for both MMU and non MMU versions.
 * there are two other files filament_sensors_handler_no_mmu, filament_sensors_handler_mmu
 * I would normally use inheritance, but last time i did that it was rewritten, so I am using this approach now.
 */

#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include <tasks.hpp>
#include <logging/log.hpp>
#include <marlin_server.hpp>
#include <option/has_crash_detection.h>
#include <option/has_selftest.h>
#include <option/has_mmu2.h>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

#include <stdio.h>

#include "str_utils.hpp"

#include <option/has_gui.h>
#include <bsod/bsod.h>
#if HAS_GUI()
    #include <window_msgbox.hpp>
#endif

#if HAS_MMU2()
    #include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#endif

#if HAS_CRASH_DETECTION()
    #include <feature/prusa/crash_recovery.hpp>
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

void FilamentSensors::request_enable_state_update() {
    enable_state_update_pending = true;
}

#if HAS_GUI()
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
#endif

void FilamentSensors::for_all_sensors(const stdext::inplace_function<void(IFSensor &sensor, uint8_t index, bool is_side)> &f) {
    for (int8_t e = 0; e < HOTENDS; e++) {
        if (IFSensor *s = GetExtruderFSensor(e)) {
            f(*s, e, false);
        }
        if (IFSensor *s = GetSideFSensor(e)) {
            f(*s, e, true);
        }
    }
}

void FilamentSensors::step() {
    static bool old_state = false;
    const bool new_state = marlin_vars().peek_fsm_states([](const auto &states) { return states.is_active(ClientFSM::Load_unload); });

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
        reconfigure_sensors_if_needed(true); // Have to be done due to autoload logical sensor on COREONE
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
    const auto new_tool = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::currently_selected());
    const auto new_tool_index = new_tool ? new_tool->to_raw() : PhysicalToolIndex::count;

    if (!force && new_tool_index == tool_index) {
        return;
    }

    tool_index = new_tool_index;

    has_mmu =
#if HAS_MMU2()
        config_store().mmu2_enabled.get(); // there is a slight semantic difference:
    // - mmu2_enabled in eeprom describes the "intent" to enable MMU on the machine (this flag is true even when the MMU is not communicating)
    // - mmu2.Enabled() is true when the MMU is operational (i.e. powered up and initial communication handshake accomplished)
    // at this spot of filament runout logic, using the "intent" is better because:
    // - you cannot start a print with MMU in the "connecting" stage
    // - when communication drops out during print, an error screen occurrs and must be resolved to continue -> runout are completely irrelevant at that moment
    // - enabling MMU in the eeprom ("intent") is an atomic operation, while it takes ~10s for the MMU to become operational
#else
        false;
#endif

    extruder_fs_independent =
#if PRINTER_IS_PRUSA_iX()
        // iX always has the "MMU rework"
        false;
#elif HAS_MMU2()
        !config_store().is_mmu_rework.get();
#else
            true;
#endif

    using LFS = LogicalFilamentSensor;
    auto &ls = logical_sensors_;

    const auto extruder_fs = new_tool ? GetExtruderFSensor(*new_tool) : nullptr;
    const auto side_fs = new_tool ? GetSideFSensor(*new_tool) : nullptr;

    const bool side_fs_enabled = side_fs && side_fs->is_enabled();

    ls[LFS::extruder] = extruder_fs;
    ls[LFS::side] = side_fs;
    ls[LFS::primary_runout] = side_fs_enabled ? side_fs : extruder_fs;
    ls[LFS::closest_to_nozzle] = extruder_fs ?: side_fs;
    ls[LFS::closest_to_nozzle_independent] = (extruder_fs && extruder_fs_independent) ? extruder_fs : side_fs;
}

void FilamentSensors::process_events() {
    if (isEvLocked()) {
        return;
    }

    const auto check_runout = [&](LogicalFilamentSensor s) {
        auto sen = sensor(s);
        if (sen == nullptr) {
            return false;
        }

        const auto event = sen->last_event();
        if (m600_sent || event != IFSensor::Event::filament_removed) {
            return false;
        }

#if HAS_CRASH_DETECTION()
        if (crash_s.get_state() != Crash_s::PRINTING) {
            // Only allow runouts in print and outside of crash recovery
            return false;
        }
#endif

        m600_sent = true;

        marlin_server::gcode_interrupt(GCodeLiteral { .gcode = "M600 A" }); // change filament

        log_info(FSensor, "Injected runout");
        return true;
    };

    const auto check_autoload = [&]() {
        const auto is_insert_event = [&](LogicalFilamentSensor s) {
            const auto si = sensor(s);
            return si && si->last_event() == IFSensor::Event::filament_inserted;
        };

        if (!(
                (
                    // Trigger autoload from extruder FS only if the filament is not missing in the side FS
                    (is_insert_event(LogicalFilamentSensor::extruder) && !no_filament_surely(LogicalFilamentSensor::side)
                        // On the MMU rework, the "FS autoload" configuration item is hidden (BFW-4290)
                        // So there's no way to enable it when it has been disabled in prior.
                        // Keep the option enabled for power users, in the spirit of the OG task assignment
                        && (config_store().fs_autoload_enabled.get() || !extruder_fs_independent))

                    // Trigger autoload from side FS only if there is no filament in the extruder
                    || (is_insert_event(LogicalFilamentSensor::side) && !has_filament_surely(LogicalFilamentSensor::extruder) && config_store().fs_autoload_enabled.get())
                    //
                    )
                && !has_mmu
                && !autoload_sent
                && !marlin_server::is_processing()
                && !isAutoloadLocked())
            //
        ) {
            return false;
        }

        autoload_sent = true;

        // autoload with return option and minimal Z value of 40mm
        marlin_server::inject(GCodeLiteral { .gcode = "M1701 Z%.2f", .parameter = Z_AXIS_LOAD_POS });

        log_info(FSensor, "Injected autoload");

        return true;
    };

    if (marlin_server::is_printing()) {
        if (check_runout(LogicalFilamentSensor::primary_runout)) {
            return;
        }

        // With an MMU, don't check for runout on the extruder sensor, it would be too late for anything
        if (!has_mmu && check_runout(LogicalFilamentSensor::extruder)) {
            return;
        }
    } else {
        // During MMU standard operation, there is no filament loaded to the nozzle when not printing.
        // So it's not a good idea to reset what filament types we have stored.
        if (!has_mmu && no_filament_surely(LogicalFilamentSensor::closest_to_nozzle) && tool_index < PhysicalToolIndex::count) {
            const auto physical_tool = PhysicalToolIndex::from_raw(tool_index);
            const auto virtual_tool = stdext::get_optional<VirtualToolIndex>(physical_tool.currently_selected_virtual_tool());
            if (virtual_tool.has_value()) {
                config_store().set_filament_type(*virtual_tool, FilamentType::none);
            }
        }

        if (check_autoload()) {
            return;
        }
    }
}

void FilamentSensors::process_enable_state_update() {
    enable_state_update_processing = true;
    enable_state_update_pending = false;

    for (int8_t e = 0; e < HOTENDS; e++) {
        if (IFSensor *s = GetExtruderFSensor(e)) {
            s->set_enabled(should_enable(s->id()));
        }
        if (IFSensor *s = GetSideFSensor(e)) {
            s->set_enabled(should_enable(s->id()));
        }
    }

#if HAS_MMU2()
    if (config_store().mmu2_enabled.get() && !config_store().fsensor_enabled.get()) {
        // MMU requires enabled filament sensor to work, turn it off if the fsensors got disabled
        marlin_server::enqueue_gcode("M709 S0");
    }
#endif

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

/**
 * @brief encode printer sensor state to MMU enum
 * TODO distinguish between at fsensor and in nozzle
 * currently only AT_FSENSOR returned
 * @return MMU2::FilamentState
 */
FilamentState FilamentSensors::WhereIsFilament() {
    switch (logical_sensor_states_[LogicalFilamentSensor::extruder]) {

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

bool hasActiveFilamentSensor(uint8_t index) {
    const auto side_sensor = GetSideFSensor(index);
    const auto ext_sensor = GetExtruderFSensor(index);

    const auto side_ok = is_fsensor_working_state(side_sensor);
    const auto ext_ok = is_fsensor_working_state(ext_sensor);

    return side_ok || ext_ok;
}

bool should_enable(FilamentSensorID id) {
#if HAS_MMU2()
    /// MMU override: when MMU is active, both extruder and side sensors
    /// are required regardless of per-sensor enable bits.
    if (config_store().mmu2_enabled.get()) {
        return true;
    }
#endif

    if (!config_store().fsensor_enabled.get()) {
        return false;
    }

    if (id.position == FilamentSensorID::Position::side) {
        return config_store().fsensor_side_enabled_bits.get() & (1 << id.index);
    } else {
        return config_store().fsensor_extruder_enabled_bits.get() & (1 << id.index);
    }
}
