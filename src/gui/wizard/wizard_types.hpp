// wizard_types.hpp
#pragma once

#include <assert.h>
#include <inttypes.h>
#include <limits>    //std::numeric_limits
#include <algorithm> //std::swap
enum class wizard_state_t {
    START_first,
    START = START_first,
    INIT,
    INFO,
    FIRST,
    START_last = FIRST,

    SELFTEST_first,
    SELFTEST_INIT = SELFTEST_first,
    SELFTEST_FAN0,
    SELFTEST_FAN1,
    SELFTEST_X,
    SELFTEST_Y,
    SELFTEST_Z,
    SELFTEST_COOL,
    SELFTEST_INIT_TEMP,
    SELFTEST_TEMP,
    SELFTEST_PASS,
    SELFTEST_FAIL,
    SELFTEST_last = SELFTEST_FAIL,

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
    XYZCALIB_PASS,
    XYZCALIB_FAIL,
    XYZCALIB_last = XYZCALIB_FAIL,

    FIRSTLAY_first,
    FIRSTLAY_INIT = FIRSTLAY_first,
    FIRSTLAY_LOAD,
    FIRSTLAY_MSBX_CALIB,
    FIRSTLAY_MSBX_START_PRINT,
    FIRSTLAY_PRINT,
    FIRSTLAY_MSBX_REPEAT_PRINT,
    FIRSTLAY_FAIL,
    FIRSTLAY_last = FIRSTLAY_FAIL,

    FINISH,

    last = FINISH
};

static_assert(int(wizard_state_t::last) < 64, "too many states in wizard_state_t");

constexpr uint64_t WizardMask(wizard_state_t state) { return (int(state) >= 64) ? std::numeric_limits<uint64_t>::max() : uint64_t(1) << int(state); }
constexpr uint64_t WizardMaskUpTo(wizard_state_t state) { return (int(state) >= 64) ? std::numeric_limits<uint64_t>::max() : WizardMask(wizard_state_t(int(state) + 1)) - uint64_t(1); }
constexpr uint64_t WizardMaskAdd(uint64_t mask, wizard_state_t state) { return mask | WizardMask(state); }
constexpr uint64_t WizardMaskRange(wizard_state_t first, wizard_state_t last) {
    if (int(first) > int(last))
        std::swap(first, last);
    return WizardMaskUpTo(last) & (~WizardMaskUpTo(first));
}

constexpr uint64_t WizardMaskStart() { return WizardMaskRange(wizard_state_t::START_first, wizard_state_t::START_last); }
constexpr uint64_t WizardMaskSelftest() { return WizardMaskRange(wizard_state_t::SELFTEST_first, wizard_state_t::SELFTEST_last) | WizardMaskStart(); }
constexpr uint64_t WizardMaskXYZCalib() { return WizardMaskRange(wizard_state_t::XYZCALIB_first, wizard_state_t::XYZCALIB_last) | WizardMaskStart(); }
constexpr uint64_t WizardMaskFirstLay() { return WizardMaskRange(wizard_state_t::FIRSTLAY_first, wizard_state_t::FIRSTLAY_last) | WizardMaskStart(); }
constexpr uint64_t WizardMaskAll() { return WizardMaskUpTo(wizard_state_t::last); }

enum {
    _SCREEN_NONE,
    _SCREEN_SELFTEST_FANS_XYZ,
    _SCREEN_SELFTEST_TEMP,
    _SCREEN_XYZCALIB_HOME,
};

enum class WizardTestState_t : uint8_t {
    START,
    RUNNING,
    PASSED,
    FAILED,
    DISABLED
};

constexpr uint64_t DidTestPass(WizardTestState_t result) {
    return (result == WizardTestState_t::PASSED) || (result == WizardTestState_t::DISABLED);
}

constexpr uint64_t IsTestDone(WizardTestState_t result) {
    return DidTestPass(result) || (result == WizardTestState_t::FAILED);
}

constexpr bool IsStateInWizardMask(wizard_state_t st, uint64_t mask) {
    return ((((uint64_t)1) << int(st)) & mask) != 0;
}

constexpr WizardTestState_t InitState(wizard_state_t st, uint64_t mask) {
    if (IsStateInWizardMask(st, mask)) {
        return WizardTestState_t::START;
    } else {
        return WizardTestState_t::DISABLED;
    }
}
