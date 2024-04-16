#pragma once

#define ENABLED(V...) false

#define DEFAULT_AXIS_STEPS_PER_UNIT \
    { 80, 80, 800, 380 }
#define DEFAULT_INVERT_X_DIR  false
#define DEFAULT_INVERT_Y_DIR  false
#define DEFAULT_INVERT_Z_DIR  true
#define DEFAULT_INVERT_E0_DIR false
#define X_CURRENT             650 // (mA) RMS current. Multiply by 1.414 for peak current.
#define X_MICROSTEPS          16 // 0..256
#define X_RSENSE              0.22
#define X_CHAIN_POS           0
#define Y_CURRENT             650
#define Y_MICROSTEPS          16
#define Y_RSENSE              0.22
#define Y_CHAIN_POS           0
#define Z_CURRENT             700
#define Z_MICROSTEPS          16
#define Z_RSENSE              0.22
#define Z_CHAIN_POS           0
#define E0_CURRENT            450
#define E0_MICROSTEPS         16
#define E0_RSENSE             0.22

typedef struct {
    float Kp, Ki, Kd;
} PID_t;
typedef struct {
    float Kp, Ki, Kd, Kc;
} PIDC_t;
