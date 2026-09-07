#pragma once
#include <utility_extensions.hpp>
#include <printer_selftest.hpp>
#include <option/has_precise_homing_corexy.h>
#include <selftest_types.hpp>
#include <utils/storage/enum_bitset.hpp>
#include <bsod/bsod.h>

namespace SelftestSnake {

// Order matters, snake and will be run in the same order, as well as menu items (with indices) will be
enum class Action : uint8_t {
    DoorSensor,
    XCheck,
    YCheck,
    ZAlign, // also known as z_calib
    BeltTuning,
#if HAS_PRECISE_HOMING_COREXY()
    PreciseHoming,
#endif
    DockCalibration,
    Loadcell, // Check loadcell before Z test, because it is used there
    ZCheck,
    Fans,
    Heaters,
    ToolOffsetsCalibration,
    NozzleCleanerCalibration,
    FilamentSensorCalibration,
    PhaseSteppingCalibration,
    InputShaper,
    _count,
    _last = _count - 1,
    _first = DoorSensor,
};

constexpr EnumBitset<Action, Action::_count> get_dependencies(Action action) {
    auto deps = EnumBitset<Action, Action::_count> {};

    // WARN: Dependencies are transitive
    // - set only direct dependencies; do not repeat what is already implied by another dependency
    // - when removing a dependency, dont forget to add back anything it was pulling in transitively
    switch (action) {
    case Action::DoorSensor:
    case Action::Fans:
        break;
    case Action::XCheck:
    case Action::YCheck:
    case Action::ZAlign:
        deps.set(Action::DoorSensor);
        break;
    case Action::BeltTuning:
        deps.set(Action::XCheck);
        deps.set(Action::YCheck);
        break;
#if HAS_PRECISE_HOMING_COREXY()
    case Action::PreciseHoming:
        deps.set(Action::BeltTuning);
        break;
#endif
    case Action::DockCalibration:
#if HAS_PRECISE_HOMING_COREXY()
        deps.set(Action::PreciseHoming);
#endif
        break;
    case Action::Heaters:
        deps.set(Action::DockCalibration);
        break;
    case Action::ToolOffsetsCalibration:
        deps.set(Action::Heaters);
        deps.set(Action::Loadcell);
        break;
    case Action::NozzleCleanerCalibration:
        deps.set(Action::ToolOffsetsCalibration);
        break;
    case Action::Loadcell:
        deps.set(Action::DockCalibration);
        // NozzleCleanerCalibration now runs later — fall back to the uncalibrated cleaner origin
        // for parking. The default position lands inside the bin (calibration only refines within
        // a few mm), so any drips still go where intended.
        break;
    case Action::FilamentSensorCalibration:
        // if filament is loaded, we need to unload (above nozzle cleaner)
        deps.set(Action::NozzleCleanerCalibration);
        break;
    case Action::ZCheck:
        deps.set(Action::Loadcell);
        deps.set(Action::ZAlign);
        break;
    case Action::PhaseSteppingCalibration:
    case Action::InputShaper:
        deps.set(Action::ZCheck);
        break;
    case Action::_count:
        debug_assert(false);
        break;
    }

    return deps;
}

TestResult get_test_result(Action action, ToolMask tool);
uint64_t get_test_mask(Action action);
} // namespace SelftestSnake
