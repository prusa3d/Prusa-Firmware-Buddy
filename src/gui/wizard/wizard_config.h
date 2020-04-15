// wizard_config.h
#ifndef _WIZARD_CONFIG_H
#define _WIZARD_CONFIG_H

#include "menu_vars.h"

//calculate move time in milliseconds (max is in mm and fr is in mm/min)
#define _SELFTEST_AXIS_TIME(max, fr) ((60 * 1000 * max) / fr)

#define _SELFTEST_FAN0_TIME 3000 // 3s
#define _SELFTEST_FAN0_MIN  100  // 100 pulses
#define _SELFTEST_FAN0_MAX  1000 // 1000 pulses

#define _SELFTEST_FAN1_TIME 3000 // 3s
#define _SELFTEST_FAN1_MIN  100  // 100 pulses
#define _SELFTEST_FAN1_MAX  1000 // 1000 pulses

#define _SELFTEST_X_MIN  (x_axis_len - len_tol_abs)
#define _SELFTEST_X_MAX  (x_axis_len + len_tol_abs)
#define _SELFTEST_X_FR   4000 // 50 mm/s
#define _SELFTEST_X_TIME (3 * _SELFTEST_AXIS_TIME(_SELFTEST_X_MAX, _SELFTEST_X_FR))

#define _SELFTEST_Y_MIN  (y_axis_len - len_tol_abs)
#define _SELFTEST_Y_MAX  (y_axis_len + len_tol_abs)
#define _SELFTEST_Y_FR   4000 // 50 mm/s
#define _SELFTEST_Y_TIME (3 * _SELFTEST_AXIS_TIME(_SELFTEST_Y_MAX, _SELFTEST_Y_FR))

#define _SELFTEST_Z_MIN  (z_axis_len - len_tol_abs)
#define _SELFTEST_Z_MAX  (z_axis_len + len_tol_abs)
#define _SELFTEST_Z_FR   600 // 10 mm/s
#define _SELFTEST_Z_TIME (2 * _SELFTEST_AXIS_TIME(_SELFTEST_Z_MAX, _SELFTEST_Z_FR))

#define _FIRSTLAY_E_DIST        100
#define _FIRSTLAY_Z_DIST        20
#define _FIRSTLAY_NOZ_TEMP      210
#define _FIRSTLAY_BED_TEMP      60
#define _FIRSTLAY_MIN_NOZ_TEMP  190
#define _FIRSTLAY_MAX_NOZ_TEMP  220
#define _FIRSTLAY_MIN_BED_TEMP  50
#define _FIRSTLAY_MAX_BED_TEMP  70
#define _FIRSTLAY_MAX_HEAT_TIME 100000

#define _CALIB_TEMP_BED    40
#define _CALIB_TEMP_NOZ    40
#define _COOLDOWN_TIMEOUT  300000
#define _START_TEMP_BED    35 //there is a bit overshot on bed PID
#define _START_TEMP_NOZ    20 //PID of nozzle is not stable with low temperatures - can be HUDGE overshot
#define _MAX_TEMP_BED      100
#define _MAX_TEMP_NOZ      280
#define _PASS_MAX_TEMP_BED 65
#define _PASS_MAX_TEMP_NOZ 190
#define _PASS_MIN_TEMP_BED 50
#define _PASS_MIN_TEMP_NOZ 130

#define _HEAT_TIME_MS_BED        60000
#define _HEAT_TIME_MS_NOZ        42000
#define _MAX_PREHEAT_TIME_MS_BED 60000
#define _MAX_PREHEAT_TIME_MS_NOZ 30000

#endif // _WIZARD_CONFIG_H
