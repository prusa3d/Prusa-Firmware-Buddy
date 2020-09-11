// wizard_types.hpp
#pragma once

#include <assert.h>
#include <inttypes.h>
#include <limits>    //std::numeric_limits
#include <algorithm> //std::swap
enum class WizardState_t {
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

    SELFTEST_AND_XYZCALIB, //SELFTEST_PASS has different message when it is combined with XYZCALIB

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
    FIRSTLAY_PASS,
    FIRSTLAY_FAIL,
    FIRSTLAY_last = FIRSTLAY_FAIL,

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

constexpr uint64_t WizardMaskStart() { return WizardMaskRange(WizardState_t::START_first, WizardState_t::START_last) | WizardMask(WizardState_t::FINISH) | WizardMask(WizardState_t::FINISH); }
constexpr uint64_t WizardMaskSelfTest() { return WizardMaskRange(WizardState_t::SELFTEST_first, WizardState_t::SELFTEST_last) | WizardMaskStart(); }
constexpr uint64_t WizardMaskXYZCalib() { return WizardMaskRange(WizardState_t::XYZCALIB_first, WizardState_t::XYZCALIB_last) | WizardMaskStart(); }
constexpr uint64_t WizardMaskSelfTestAndXYZCalib() { //SELFTEST_PASS has different message when it is combined with XYZCALIB
    return (WizardMaskSelfTest() | WizardMaskXYZCalib() | WizardMask(WizardState_t::SELFTEST_AND_XYZCALIB)) & ~WizardMask(WizardState_t::SELFTEST_PASS);
}
constexpr uint64_t WizardMaskFirstLay() { return WizardMaskRange(WizardState_t::FIRSTLAY_first, WizardState_t::FIRSTLAY_last) | WizardMaskStart(); }
constexpr uint64_t WizardMaskAll() { return WizardMaskUpTo(WizardState_t::last); }

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

constexpr bool IsStateInWizardMask(WizardState_t st, uint64_t mask) {
    return ((((uint64_t)1) << int(st)) & mask) != 0;
}

constexpr WizardTestState_t InitState(WizardState_t st, uint64_t mask) {
    if (IsStateInWizardMask(st, mask)) {
        return WizardTestState_t::START;
    } else {
        return WizardTestState_t::DISABLED;
    }
}

class StateFncData {
    WizardState_t next_state;
    WizardTestState_t result;

public:
    WizardState_t GetState() { return next_state; }
    WizardTestState_t GetResult() { return result; }
    StateFncData PassToNext() { return StateFncData(GetNextWizardState(GetState()), WizardTestState_t::PASSED); }
    StateFncData(WizardState_t state, WizardTestState_t res)
        : next_state(state)
        , result(res) {}
};
