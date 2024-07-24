// config.h - main configuration file
#pragma once

#include "printers.h"
#include <stdint.h>
#include <array>
#include <cassert>

//************************
//*** System configuration

#define HEATBEDLET_COUNT     16 // please do not change this value
#define TICKS_PER_SECOND     1000 // please do not change this value
#define DEFAULT_CHAMBER_TEMP 25.0f

//***********************
//*** RS485 configuration

#define RS485_BUFFER_SIZE 256 // Modbus specification needs 256 Bytes

#define RS485_BAUDRATE  230400
#define RS485_STOP_BITS UART_STOPBITS_1
#define RS485_PARITY    UART_PARITY_NONE

#define RS485_ASSERTION_TIME   16
#define RS485_DEASSERTION_TIME 16

// Modbus specification requires at least 3.5 char delay between frames, one character uses 11 bits
#define RS485_RX_TIMEOUT_BITS 33

#define RS485_DE_SIGNAL_GPIO_PORT GPIOD
#define RS485_DE_SIGNAL_GPIO_PIN  GPIO_PIN_6

//************************
//*** Modbus configuration

#define MODBUS_WATCHDOG_TIMEOUT            30000
#define MODBUS_OTP_FILE_NUMBER             1
#define MODBUS_TEMPERATURE_REGISTERS_SCALE 10
#define MODBUS_CURRENT_REGISTERS_SCALE     1000
#define MODBUS_PWM_STATE_SCALE             1000
#define MODBUS_PID_SCALE                   10000
#define MODBUS_FIFO_MAX_COUNT              31 // Same value as MODBUS_FIFO_LEN defined in fifo_coder.hpp, maximal value allowed by Modbus specification is 31

//*****************************
//*** Measurement configuration

#define ADC_CONVERSION_PERIOD 3 // 3 ms * 32 measurements -> 10.4 Hz frequency for each channel
inline constexpr uint8_t ADC_BATCH_SIZE = 34; // please do not change this value
#define ADC_REFERENCE_VOLTAGE 3.3f

#define HEATBEDLET_THERMISTOR_VOLTAGE_DIVIDER_RESISTANCE 4700
#define HEATBEDLET_THERMISTOR_RESISTANCE_AT_T_ZERO       100000
#define HEATBEDLET_THERMISTOR_T_ZERO                     25
#define HEATBEDLET_THERMISTOR_B_CONSTANT                 4573 // range 25/100 °C; tolerance 3%

#define HEATBEDLET_VOLTAGE                            24
#define HEATBEDLET_RESISTANCE_TEMPERATURE_COEFFICIENT 0.0042f // measured on real heatbedlets
#define HEATBEDLET_REFERENCE_TEMPERATURE              20.0f
#define HEATBEDLET_DEFAULT_RESISTANCE                 14.0f // expect low resistance to be on the safe side
#define HEATBEDLET_DEFAULT_MAX_ALLOWED_CURRENT        1.5f

#define CURRENT_SENSOR_SENSITIVITY_MILLIVOLTS_PER_AMPERE 90
#define CURRENT_SENSOR_SENSITIVITY_REFERENCE_VOLTAGE     3.3f
#define CURRENT_SENSOR_REFERENCE_VOLTAGE                 3.3f
#define CURRENT_FILTER_DATA_POINT_COUNT                  80 // Number of data points for moving average filter. ADC performs 83 measurements per second for each current cannel

//*******************************
//*** Safety checks configuration

#define ENABLE_TEMPERATURE_CHECKS 1
#define TURN_OFF_HEATING_ON_ERROR 1

#define CURRENT_MIN 0.1f

#define TEMPERATURE_MIN 5
#define TEMPERATURE_MAX 120

#define TEMPERATURE_STABILIZED_TOLERANCE_DEGREES 1
#define TEMPERATURE_STABILIZED_MIN_SECONDS       4

#define TEMPERATURE_DROP_THRESHOLD_WITH_FAN_DEGREES    35 // when fan is blowing on one particular heatbedlet, it can cool down significantly
#define TEMPERATURE_DROP_THRESHOLD_WITHOUT_FAN_DEGREES 10
#define TEMPERATURE_DROP_THRESHOLD_SECONDS             360

#define TEMPERATURE_PEAK_THRESHOLD_DEGREES 3
#define TEMPERATURE_PEAK_THRESHOLD_SECONDS 60
#define TEMPERATURE_PEAK_AMBIENT_DEGREES   50

#define TEMPERATURE_PREHEAT_CHECK_MIN_DEGREES_WITHOUT_FAN 0.5 // 0.5 degree increase per minute
#define TEMPERATURE_PREHEAT_CHECK_MIN_DEGREES_WITH_FAN    -20 // With fan, we allow temperature to decrease
#define TEMPERATURE_PREHEAT_CHECK_SECONDS                 60

namespace Branch {
inline constexpr uint8_t A = 0;
inline constexpr uint8_t B = 1;
inline constexpr auto count = 2;
inline void assert_idx([[maybe_unused]] const uint8_t idx) {
    assert(idx == A || idx == B);
}
}; // namespace Branch

inline constexpr std::array<float, Branch::count> OVERCURRENT_THRESHOLD_AMPS { 10.5f, 6.3f };
inline constexpr std::array<float, Branch::count> UNEXPECTED_CURRENT_TOLERANCE { 2.0f, 1.2f };

#define MIN_HB_RESISTANCE 4.5f // for checking of "HeaterShortCircuit" error
#define MAX_HB_RESISTANCE 40.0f // for checking of "HeaterDisconnected" error
#define INF_RESISTANCE    (MAX_HB_RESISTANCE * 1000)

#if PRINTER_IS_PRUSA_iX()
inline constexpr bool is_used_bedlet(uint32_t heatbedletIndex) {
    uint32_t connector_nr = heatbedletIndex + 1;
    switch (connector_nr) {
    case 1:
    case 7:
    case 8:
    case 9:
    case 10:
    case 15:
    case 16:
        return false;
    default:
        return true;
    }
}
inline constexpr std::array<float, Branch::count> PWM_MAX_CURRENT_AMPS { 4, 5 }; // {4x HeatBedLet on 24V_A, 5x HeatBedLet on 24V_B}
#elif PRINTER_IS_PRUSA_XL()
inline constexpr bool is_used_bedlet(uint32_t) {
    return true;
}
inline constexpr std::array<float, Branch::count> PWM_MAX_CURRENT_AMPS { 10, 6 };
#else
    #error "unknown printer"
#endif

//*********************
//*** PWM configuration

#define PWM_TIMER_FREQUENCY 10000 // average duration of single IRQ handler call is 1.62 microseconds
#define PWM_PERIOD_LENGTH   256

#define PWM_RAMPING_START         0.85f // defines starting point of current ramping; value means fraction of maximal PSU current
#define PWM_RAMPING_END           0.95f // defines ending point of current ramping; should be < 1 to allow some margin under maximal current; value means fraction of maximal PSU current
#define PWM_OVERCURRENT_THRESHOLD 0.96f // when current overshoots this threshold, then PWM is instantly limited to PWM_RAMPING_START; value means fraction of maximal PSU current
#define PWM_RAMPING_SPEED         0.002f // ramping speed of PWM when current is close to it's maximum; value is dimensionless coefficient, it was determined experimentally

//********************
//*** Autoconfigurtion

#define AUTOCONFIG_HB_HEATING_PWM_PERCENT           80
#define AUTOCONFIG_HB_HEATING_SECONDS               15
#define AUTOCONFIG_HB_HEATING_MIN_TEMPERATURE_RAISE 1.0f

//***************
//*** Controllers

#define CONTROLLER_FREQUENCY               5
#define CONTROLLER_TEMP_FILTER_LENGTH      2 // history size for moving average of temperature
#define CONTROLLER_TEMP_DIFF_FILTER_LENGTH 14 // history size for moving average of temperature differential

#define CONTROLLER_EXPECTED_PWM_COEF_A1 0.0105f // measured on real heatbedlets

#define CONTROLLER_PWM_DELINEAR_COEF_A2 0.553083f // measured on real heatbedlets
#define CONTROLLER_PWM_DELINEAR_COEF_A1 0.446917f // measured on real heatbedlets

#define CONTROLLER_P_COEF 0.1800f
#define CONTROLLER_I_COEF 0.0030f
#define CONTROLLER_D_COEF 0.2000f
