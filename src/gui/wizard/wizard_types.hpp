// wizard_types.hpp
#pragma once

#include <assert.h>
#include <inttypes.h>
#include <limits>    //std::numeric_limits
#include <algorithm> //std::swap
enum class WizardState_t {
    repeat = -2,
    next = -1,
    first = 0,

    START_first = first,
    START = START_first,
    INIT,
    INFO,
    FIRST,
    START_last = FIRST,

    SELFTEST_first,
    SELFTEST_FAN = SELFTEST_first,
    SELFTEST_X,
    SELFTEST_Y,
    SELFTEST_Z,
    SELFTEST_XYZ,
    SELFTEST_TEMP,
    SELFTEST_RESULT,
    SELFTEST_last = SELFTEST_RESULT,

    SELFTEST_AND_XYZCALIB, //SELFTEST_RESULT has different message when it is combined with XYZCALIB

    XYZCALIB_first,
    XYZCALIB_INIT = XYZCALIB_first,
    XYZCALIB_HOME,
    XYZCALIB_Z,
    XYZCALIB_XY_MSG_CLEAN_NOZZLE,
    XYZCALIB_XY_MSG_IS_SHEET,
    XYZCALIB_XY_MSG_REMOVE_SHEET,
    XYZCALIB_XY_MSG_PLACE_PAPER,
    XYZCALIB_XY_SEARCH,
    XYZCALIB_XY_MSG_PLACE_SHEET,
    XYZCALIB_XY_MEASURE,
    XYZCALIB_RESULT,
    XYZCALIB_last = XYZCALIB_RESULT,

    FIRSTLAY_first,
    FIRSTLAY_FILAMENT_ASK = FIRSTLAY_first,
    FIRSTLAY_FILAMENT_ASK_PREHEAT,
    FIRSTLAY_FILAMENT_LOAD,
    FIRSTLAY_FILAMENT_UNLOAD,
    FIRSTLAY_MSBX_CALIB,
    FIRSTLAY_MSBX_USEVAL,
    FIRSTLAY_MSBX_START_PRINT,
    FIRSTLAY_PRINT,
    FIRSTLAY_MSBX_REPEAT_PRINT,
    FIRSTLAY_RESULT,
    FIRSTLAY_last = FIRSTLAY_RESULT,

    FINISH,
    EXIT,

    last = EXIT
};

static_assert(int(WizardState_t::last) < 64, "too many states in wizard_state_t");

constexpr WizardState_t GetNextWizardState(WizardState_t state) { return state == WizardState_t::last ? WizardState_t::last : WizardState_t(int(state) + 1); }
constexpr uint64_t WizardMask(WizardState_t state) { return (int(state) >= 64) ? std::numeric_limits<uint64_t>::max() : uint64_t(1) << int(state); }
constexpr uint64_t WizardMaskUpTo(WizardState_t state) { return (int(state) >= 64) ? std::numeric_limits<uint64_t>::max() : WizardMask(WizardState_t(int(state) + 1)) - uint64_t(1); }
constexpr uint64_t WizardMaskAdd(uint64_t mask, WizardState_t state) { return mask | WizardMask(state); }
constexpr uint64_t WizardMaskRange(WizardState_t first, WizardState_t last) {
    if (int(first) > int(last))
        std::swap(first, last);
    return WizardMaskUpTo(last) & ((~WizardMaskUpTo(first)) | WizardMask(first));
}

constexpr uint64_t WizardMaskStart() { return WizardMaskRange(WizardState_t::START_first, WizardState_t::START_last) | WizardMask(WizardState_t::EXIT) | WizardMask(WizardState_t::EXIT); }
constexpr uint64_t WizardMaskSelfTest() {
    return (WizardMaskRange(WizardState_t::SELFTEST_first, WizardState_t::SELFTEST_last) | WizardMask(WizardState_t::EXIT) | WizardMask(WizardState_t::EXIT) /*| WizardMaskStart()*/)
        & ~WizardMaskRange(WizardState_t::SELFTEST_X, WizardState_t::SELFTEST_Z); //exclude standalone axis tests
}
constexpr uint64_t WizardMaskXYZCalib() { return WizardMaskRange(WizardState_t::XYZCALIB_first, WizardState_t::XYZCALIB_last) | WizardMaskStart(); }
constexpr uint64_t WizardMaskSelfTestAndXYZCalib() { //SELFTEST_RESULT has different message when it is combined with XYZCALIB
    return (WizardMaskSelfTest() | WizardMaskXYZCalib() | WizardMask(WizardState_t::SELFTEST_AND_XYZCALIB)) & ~WizardMask(WizardState_t::SELFTEST_RESULT);
}
constexpr uint64_t WizardMaskFirstLay() {
    return WizardMaskRange(WizardState_t::FIRSTLAY_first, WizardState_t::FIRSTLAY_last) | WizardMask(WizardState_t::EXIT) | WizardMask(WizardState_t::EXIT) /* | WizardMaskStart()*/;
}

//disabled XYZ calib
constexpr uint64_t WizardMaskAll() { return WizardMaskStart() | WizardMaskSelfTest() | WizardMaskFirstLay(); }

constexpr bool IsStateInWizardMask(WizardState_t st, uint64_t mask) {
    return ((((uint64_t)1) << int(st)) & mask) != 0;
}
