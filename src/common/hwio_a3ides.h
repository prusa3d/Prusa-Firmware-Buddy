// hwio_a3ides.h

#pragma once

//a3ides digital inputs
enum {
    _DI_Z_MIN = 0, // PA8
    _DI_E_DIAG,    // PA15
    _DI_Y_DIAG,    // PE1
    _DI_X_DIAG,    // PE2
    _DI_Z_DIAG,    // PE3
    _DI_BTN_ENC,   // PE12
    _DI_BTN_EN1,   // PE13
    _DI_BTN_EN2,   // PE15
};

//a3ides digital outputs
enum {
    _DO_X_DIR = 0, // PD0
    _DO_X_STEP,    // PD1
    _DO_Z_ENABLE,  // PD2
    _DO_X_ENABLE,  // PD3
    _DO_Z_STEP,    // PD4
    _DO_E_DIR,     // PD8
    _DO_E_STEP,    // PD9
    _DO_E_ENABLE,  // PD10
    _DO_Y_DIR,     // PD12
    _DO_Y_STEP,    // PD13
    _DO_Y_ENABLE,  // PD14
    _DO_Z_DIR,     // PD15
};

//a3ides pwm outputs
enum {
    _PWM_HEATER_BED = 0,
    _PWM_HEATER_0,
    _PWM_FAN1,
    _PWM_FAN,
};

//a3ides fan control
enum {
    _FAN = 0,
    _FAN1,
};

//a3ides heater control
enum {
    _HEATER_0 = 0,
    _HEATER_BED,
};
