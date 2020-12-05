// wizard_config.hpp
#pragma once
#include "eeprom.h" // SelftestResult_Passed, SelftestResult_Failed

enum class SelftestSubtestState_t : uint8_t { //it is passed as uint8_t between threads
    undef,
    ok,
    not_good,
    running
};

constexpr SelftestSubtestState_t SelftestStateFromEeprom(uint8_t state) {
    switch (state) {
    case SelftestResult_Passed:
        return SelftestSubtestState_t::ok;
    case SelftestResult_Failed:
        return SelftestSubtestState_t::not_good;
    default:
        return SelftestSubtestState_t::undef;
    }
}

enum {
    WIZARD_MARGIN_LEFT = 6,
    WIZARD_MARGIN_RIGHT = 6,
    WIZARD_X_SPACE = 240 - (WIZARD_MARGIN_LEFT + WIZARD_MARGIN_RIGHT),

    MSGBOX_BTN_NEXT = /*MSGBOX_BTN_MAX*/ +1,
    MSGBOX_BTN_DONE = /*MSGBOX_BTN_MAX*/ +2,
};

//calculate move time in milliseconds (max is in mm and fr is in mm/min)
#define _SELFTEST_AXIS_TIME(max, fr) ((60 * 1000 * max) / fr)

#define _SELFTEST_X_MIN  (x_axis_len - len_tol_abs)
#define _SELFTEST_X_MAX  (x_axis_len + len_tol_abs)
#define _SELFTEST_X_TIME (3 * _SELFTEST_AXIS_TIME(_SELFTEST_X_MAX, _SELFTEST_X_FR))

#define _SELFTEST_Y_MIN  (y_axis_len - len_tol_abs)
#define _SELFTEST_Y_MAX  (y_axis_len + len_tol_abs)
#define _SELFTEST_Y_TIME (3 * _SELFTEST_AXIS_TIME(_SELFTEST_Y_MAX, _SELFTEST_Y_FR))

#define _SELFTEST_Z_MIN  (z_axis_len - len_tol_abs)
#define _SELFTEST_Z_MAX  (z_axis_len + len_tol_abs)
#define _SELFTEST_Z_TIME (2 * _SELFTEST_AXIS_TIME(_SELFTEST_Z_MAX, _SELFTEST_Z_FR))

enum {
    _SELFTEST_FAN0_TIME = 3000, // 3s
    _SELFTEST_FAN0_MIN = 100,   // 100 pulses
    _SELFTEST_FAN0_MAX = 1000,  // 1000 pulses

    _SELFTEST_FAN1_TIME = 3000, // 3s
    _SELFTEST_FAN1_MIN = 100,   // 100 pulses
    _SELFTEST_FAN1_MAX = 1000,  // 1000 pulses

    _SELFTEST_X_FR = 4000, // 50 mm/s

    _SELFTEST_Y_FR = 4000, // 50 mm/s

    _SELFTEST_Z_FR = 600, // 10 mm/s

    _FIRSTLAY_E_DIST = 100,
    _FIRSTLAY_Z_DIST = 20,
    _FIRSTLAY_NOZ_TEMP = 210,
    _FIRSTLAY_BED_TEMP = 60,
    _FIRSTLAY_MIN_NOZ_TEMP = 190,
    _FIRSTLAY_MAX_NOZ_TEMP = 220,
    _FIRSTLAY_MIN_BED_TEMP = 50,
    _FIRSTLAY_MAX_BED_TEMP = 70,
    _FIRSTLAY_MAX_HEAT_TIME = 100000,

    _CALIB_TEMP_BED = 40,
    _CALIB_TEMP_NOZ = 40,
    _COOLDOWN_TIMEOUT = 300000,
    _START_TEMP_BED = 35, //there is a bit overshot on bed PID
    _START_TEMP_NOZ = 20, //PID of nozzle is not stable with low temperatures - can be HUDGE overshot
    _MAX_TEMP_BED = 100,
    _MAX_TEMP_NOZ = 280,
    _PASS_MAX_TEMP_BED = 65,
    _PASS_MAX_TEMP_NOZ = 190,
    _PASS_MIN_TEMP_BED = 50,
    _PASS_MIN_TEMP_NOZ = 130,

    _HEAT_TIME_MS_BED = 60000,
    _HEAT_TIME_MS_NOZ = 42000,
    _MAX_PREHEAT_TIME_MS_BED = 60000,
    _MAX_PREHEAT_TIME_MS_NOZ = 30000,
};
