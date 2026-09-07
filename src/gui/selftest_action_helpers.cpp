#include "selftest_action_helpers.hpp"

#include <option/has_precise_homing_corexy.h>
#include <option/has_door_sensor_calibration.h>
#include <option/has_loadcell.h>
#include <option/has_gearbox_alignment.h>
#include <option/has_phase_stepping_selftest.h>
#include <option/has_indx.h>
#include <option/has_manual_belt_tuning.h>
#include <option/has_toolchanger.h>
#include <bsod.h>

namespace SelftestSnake {

static Action _get_valid_action(Action start_action, int step) {
    debug_assert(step == 1 || step == -1); // other values would cause weird behaviour (endless loop / go beyond array)
    if (is_multitool()) {
        while (is_singletool_only_action(start_action)) {
            start_action = static_cast<Action>(std::to_underlying(start_action) + step);
        }
    } else { // singletool
        while (is_multitool_only_action(start_action)) {
            start_action = static_cast<Action>(std::to_underlying(start_action) + step);
        }
    }
    return start_action;
}

Action get_first_action() {
    return _get_valid_action(Action::_first, 1);
}

Action get_last_action() {
    return _get_valid_action(Action::_last, -1);
}

Action get_next_action(Action action) {
    debug_assert(get_last_action() != action && "Unhandled edge case");
    return _get_valid_action(static_cast<Action>(std::to_underlying(action) + 1), 1);
}

ValidActionsRange::Iterator &ValidActionsRange::Iterator::operator++() {
    action_ = _get_valid_action(static_cast<Action>(std::to_underlying(action_) + 1), 1);
    return *this;
}

const char *get_action_label(Action action) {
    switch (action) {
    case Action::Fans:
        return N_("Fan Test");
    case Action::ZCheck:
        return N_("Z Axis Test");
    case Action::Heaters:
        return N_("Heater Test");
    case Action::FilamentSensorCalibration:
        return N_("Filament Sensor Calibration");
#if !PRINTER_IS_PRUSA_MINI()
    case Action::ZAlign:
        return N_("Z Alignment Calibration");
#endif
#if PRINTER_IS_PRUSA_MINI() || PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_MK4()
    case Action::XYCheck:
        return N_("XY Axis Test");
#else
    case Action::XCheck:
        return N_("X Axis Test");
    case Action::YCheck:
        return N_("Y Axis Test");
#endif
#if PRINTER_IS_PRUSA_MINI() || PRINTER_IS_PRUSA_MK3_5()
    case Action::FirstLayer:
        return N_("First Layer Calibration");
#endif
#if HAS_PRECISE_HOMING_COREXY()
    case Action::PreciseHoming:
        return N_("Homing Calibration");
#endif
#if HAS_DOOR_SENSOR_CALIBRATION()
    case Action::DoorSensor:
        return N_("Door Sensor");
#endif
#if HAS_LOADCELL()
    case Action::Loadcell:
        return N_("Loadcell Test");
#endif
#if HAS_GEARBOX_ALIGNMENT()
    case Action::Gears:
        return N_("Gearbox Alignment");
#endif
#if HAS_PHASE_STEPPING_SELFTEST()
    case Action::PhaseSteppingCalibration:
        return N_("Phase Stepping Calibration");
#endif
#if HAS_MANUAL_BELT_TUNING()
    case Action::BeltTuning:
        return N_("Belt Tuning");
#endif
#if HAS_INDX()
    case Action::InputShaper:
        return N_("Input Shaper Calibration");
#endif
#if HAS_TOOLCHANGER()
    case Action::DockCalibration:
    #if HAS_INDX()
        return N_("Dock Calibration");
    #else
        return N_("Dock Position Calibration");
    #endif
#endif
#if HAS_INDX()
    case Action::NozzleCleanerCalibration:
        return N_("Nozzle Cleaner Calibration");
    case Action::ToolOffsetsCalibration:
        return N_("Tool Offsets Calibration");
#endif
#if PRINTER_IS_PRUSA_XL()
    case Action::ToolOffsetsCalibration:
        return N_("Tool Offset Calibration");
    case Action::BedHeaters:
        return N_("Bed Heater Test");
    case Action::NozzleHeaters:
        return N_("Nozzle Heaters Test");
#endif
    case Action::_count:
        debug_assert(false);
        return "";
    }
    bsod_unreachable();
}

} // namespace SelftestSnake
