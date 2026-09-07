#pragma once
#include <utility_extensions.hpp>
#include <printer_selftest.hpp>
#include <selftest_types.hpp>
#include <option/has_precise_homing_corexy.h>

namespace SelftestSnake {

// Order matters, snake and will be run in the same order, as well as menu items (with indices) will be
enum class Action {
    Fans,
    YCheck,
    XCheck,
#if HAS_PRECISE_HOMING_COREXY()
    PreciseHoming,
#endif
    ZAlign, // also known as z_calib
    Loadcell, // Check loadcell before Z test, because it is used there
    ZCheck,
    Heaters,
    FilamentSensorCalibration,
    PhaseSteppingCalibration,
    BeltTuning,
    _count,
    _last = _count - 1,
    _first = Fans,
};

TestResult get_test_result(Action action, ToolMask tool);
uint64_t get_test_mask(Action action);
} // namespace SelftestSnake
