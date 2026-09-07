/// @file
#pragma once

#include <inc/MarlinConfig.h>
#include <module/thermistor/thermistors.h>

#if HOTENDS <= 1
    #define HOTEND_INDEX 0
    #define E_NAME
#else
    #define HOTEND_INDEX e
    #define E_NAME       e
#endif

// Identifiers for other heaters
enum heater_ind_t : int8_t {
    INDEX_NONE = -4,
    H_REDUNDANT,
    H_BOARD,
    H_BED,
    H_NOZZLE_FIRST,
    H_NOZZLE_LAST = H_NOZZLE_FIRST + HOTENDS - 1,
    H_HEATBREAK_FIRST,
    H_HEATBREAK_LAST = H_HEATBREAK_FIRST + HOTENDS - 1,
};
static_assert(H_NOZZLE_FIRST == 0); // lots of places in are indexed by this, and assumes H_NOZZLE_FIRST is zero

/// A bold assumption used by steady_state_hotend and temp regulator
static constexpr float ambient_temp = 21.0f;

// PID storage
struct PID_t {
    float Kp = 0, Ki = 0, Kd = 0;
};

// A temperature sensor
typedef struct TempInfo {
    static constexpr float celsius_uninitialized = -1.0f;

    uint16_t acc;
    int16_t raw;
    float celsius = celsius_uninitialized;
    inline void reset() { acc = 0; }
    inline void sample(const uint16_t s) { acc += s; }
    inline void update() { raw = acc; }
} temp_info_t;

// Minimum number of Temperature::ISR loops between sensor readings.
// Multiplied by 16 (OVERSAMPLENR) to obtain the total time to
// get all oversampled sensor readings
#define MIN_ADC_ISR_LOOPS 10

/**
 * States for ADC reading in the ISR
 */
enum ADCSensorState : char {
    StartSampling,
#if HAS_TEMP_ADC_0
    MeasureTemp_0,
#endif
#if HAS_LOCAL_BED()
    MeasureTemp_BED,
#endif
#if HAS_TEMP_HEATBREAK
    MeasureTemp_HEATBREAK,
#endif
#if HAS_TEMP_BOARD
    MeasureTemp_BOARD,
#endif
#if PRINTER_IS_PRUSA_iX()
    MeasureTemp_PSU,
    MeasureTemp_AMBIENT,
#endif
    SensorsReady, // Temperatures ready. Delay the next round of readings to let ADC pins settle.
    StartupDelay
};

#define ACTUAL_ADC_SAMPLES _MAX(int(MIN_ADC_ISR_LOOPS), int(SensorsReady))

#define PID_dT ((OVERSAMPLENR * float(ACTUAL_ADC_SAMPLES)) / TEMP_TIMER_FREQUENCY)

// Apply the scale factors to the PID values
#define scalePID_i(i)   (float(i) * PID_dT)
#define unscalePID_i(i) (float(i) / PID_dT)
#define scalePID_d(d)   (float(d) / PID_dT)
#define unscalePID_d(d) (float(d) * PID_dT)
