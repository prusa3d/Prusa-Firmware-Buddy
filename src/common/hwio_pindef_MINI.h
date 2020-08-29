//----------------------------------------------------------------------------//
// pin names
#pragma once

#include "gpio.h"

enum {
    PIN_BEEPER = TPA0,
    PIN_HW_IDENTIFY = TPA3,
    PIN_TEMP_BED = TPA4,
    PIN_THERM2 = TPA5,
    PIN_TEMP_HEATBREAK = TPA6,
    PIN_Z_MIN = TPA8,
    PIN_HEATER_BED = TPB0,
    PIN_HEATER_0 = TPB1,
    PIN_TEMP_0 = TPC0,
    PIN_X_DIR = TPD0,
    PIN_X_STEP = TPD1,
    PIN_X_ENABLE = TPD3,
    PIN_X_DIAG = TPE2,
    PIN_Y_DIR = TPD12,
    PIN_Y_STEP = TPD13,
    PIN_Y_ENABLE = TPD14,
    PIN_Y_DIAG = TPE1,
    PIN_Z_DIR = TPD15,
    PIN_Z_STEP = TPD4,
    PIN_Z_ENABLE = TPD2,
    PIN_Z_DIAG = TPE3,
    PIN_E_DIR = TPD8,
    PIN_E_STEP = TPD9,
    PIN_E_ENABLE = TPD10,
    PIN_E_DIAG = TPA15,
    PIN_FAN1 = TPE9,
    PIN_FAN = TPE11,
    PIN_BTN_ENC = TPE12,
    PIN_BTN_EN1 = TPE13,
    PIN_BTN_EN2 = TPE15,
    PIN_FSENSOR = TPB4,
};
