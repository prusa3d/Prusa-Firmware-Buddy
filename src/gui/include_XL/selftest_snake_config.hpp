#pragma once
#include <utility_extensions.hpp>
#include <printer_selftest.hpp>
#include <option/has_precise_homing_corexy.h>
#include <selftest_types.hpp>
#include <utils/storage/enum_bitset.hpp>
#include <bsod/bsod.h>

namespace SelftestSnake {

// Order matters, snake and will be run in the same order, as well as menu items (with indices) will be
enum class Action {
    Fans,
    Heaters,
    NozzleHeaters,
    YCheck,
    XCheck,
#if HAS_PRECISE_HOMING_COREXY()
    PreciseHoming,
#endif
    DockCalibration,
    Gears,
    Loadcell,
    FilamentSensorCalibration,
    ZAlign,
    ZCheck,
    ToolOffsetsCalibration,
    BedHeaters,
    PhaseSteppingCalibration,
    _count,
    _last = _count - 1,
    _first = Fans,
};

constexpr EnumBitset<Action, Action::_count> get_dependencies(Action action) {
    auto deps = EnumBitset<Action, Action::_count> {};

    // WARN: Dependencies are transitive
    // - set only direct dependencies; do not repeat what is already implied by another dependency
    // - when removing a dependency, dont forget to add back anything it was pulling in transitively
    switch (action) {
    case Action::Fans:
    case Action::YCheck:
    case Action::XCheck:
        break;
    case Action::Heaters:
    case Action::NozzleHeaters:
    case Action::BedHeaters:
        deps.set(Action::Fans);
        break;
#if HAS_PRECISE_HOMING_COREXY()
    case Action::PreciseHoming:
        deps.set(Action::XCheck);
        deps.set(Action::YCheck);
        break;
#endif
    case Action::DockCalibration:
        deps.set(Action::PreciseHoming);
        break;
    case Action::Gears:
    case Action::Loadcell:
    case Action::ZAlign:
        deps.set(Action::DockCalibration);
        break;
    case Action::ZCheck:
        deps.set(Action::ZAlign);
        break;
    case Action::FilamentSensorCalibration:
        deps.set(Action::NozzleHeaters); // may need to unload filament
        break;
    case Action::ToolOffsetsCalibration:
        deps.set(Action::Loadcell);
        deps.set(Action::NozzleHeaters); // measurement is done with hot nozzle
        break;
    case Action::PhaseSteppingCalibration:
        deps.set(Action::DockCalibration);
        deps.set(Action::ZCheck); // PS calibration may move with bed
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
