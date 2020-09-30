//----------------------------------------------------------------------------//
// pin names
#pragma once

#include "Pin.hpp"

#define MARLIN_PORT_Z_MIN   MARLIN_PORT_A
#define MARLIN_PIN_NR_Z_MIN MARLIN_PIN_NR_8

#define MARLIN_PORT_X_DIAG   MARLIN_PORT_E
#define MARLIN_PIN_NR_X_DIAG MARLIN_PIN_NR_2

#define MARLIN_PORT_Y_DIAG   MARLIN_PORT_E
#define MARLIN_PIN_NR_Y_DIAG MARLIN_PIN_NR_1

#define MARLIN_PORT_Z_DIAG   MARLIN_PORT_E
#define MARLIN_PIN_NR_Z_DIAG MARLIN_PIN_NR_3

#define MARLIN_PORT_E0_DIAG   MARLIN_PORT_A
#define MARLIN_PIN_NR_E0_DIAG MARLIN_PIN_NR_15

#define MARLIN_PORT_X_STEP   MARLIN_PORT_D
#define MARLIN_PIN_NR_X_STEP MARLIN_PIN_NR_1

#define MARLIN_PORT_X_DIR   MARLIN_PORT_D
#define MARLIN_PIN_NR_X_DIR MARLIN_PIN_NR_0

#define MARLIN_PORT_X_ENA   MARLIN_PORT_D
#define MARLIN_PIN_NR_X_ENA MARLIN_PIN_NR_3

#define MARLIN_PORT_Y_STEP   MARLIN_PORT_D
#define MARLIN_PIN_NR_Y_STEP MARLIN_PIN_NR_13

#define MARLIN_PORT_Y_DIR   MARLIN_PORT_D
#define MARLIN_PIN_NR_Y_DIR MARLIN_PIN_NR_12

#define MARLIN_PORT_Y_ENA   MARLIN_PORT_D
#define MARLIN_PIN_NR_Y_ENA MARLIN_PIN_NR_14

#define MARLIN_PORT_Z_STEP   MARLIN_PORT_D
#define MARLIN_PIN_NR_Z_STEP MARLIN_PIN_NR_4

#define MARLIN_PORT_Z_DIR   MARLIN_PORT_D
#define MARLIN_PIN_NR_Z_DIR MARLIN_PIN_NR_15

#define MARLIN_PORT_Z_ENA   MARLIN_PORT_D
#define MARLIN_PIN_NR_Z_ENA MARLIN_PIN_NR_2

#define MARLIN_PORT_E0_STEP   MARLIN_PORT_D
#define MARLIN_PIN_NR_E0_STEP MARLIN_PIN_NR_9

#define MARLIN_PORT_E0_DIR   MARLIN_PORT_B
#define MARLIN_PIN_NR_E0_DIR MARLIN_PIN_NR_8

#define MARLIN_PORT_E0_ENA   MARLIN_PORT_D
#define MARLIN_PIN_NR_E0_ENA MARLIN_PIN_NR_10

#define MARLIN_PORT_BED_HEAT   MARLIN_PORT_B
#define MARLIN_PIN_NR_BED_HEAT MARLIN_PIN_NR_0

#define MARLIN_PORT_HEAT0   MARLIN_PORT_B
#define MARLIN_PIN_NR_HEAT0 MARLIN_PIN_NR_1

#define MARLIN_PORT_FAN   MARLIN_PORT_E
#define MARLIN_PIN_NR_FAN MARLIN_PIN_NR_11

#define MARLIN_PORT_FAN1   MARLIN_PORT_E
#define MARLIN_PIN_NR_FAN1 MARLIN_PIN_NR_9

#define MARLIN_PORT_HW_IDENTIFY   MARLIN_PORT_A
#define MARLIN_PIN_NR_HW_IDENTIFY MARLIN_PIN_NR_3 //ADC, unused

#define MARLIN_PORT_TEMP_BED   MARLIN_PORT_A
#define MARLIN_PIN_NR_TEMP_BED MARLIN_PIN_NR_4 //ADC

#define MARLIN_PORT_THERM2   MARLIN_PORT_A
#define MARLIN_PIN_NR_THERM2 MARLIN_PIN_NR_5 //ADC

#define MARLIN_PORT_TEMP_HEATBREAK   MARLIN_PORT_A
#define MARLIN_PIN_NR_TEMP_HEATBREAK MARLIN_PIN_NR_6 //ADC

#define MARLIN_PORT_TEMP_0   MARLIN_PORT_C
#define MARLIN_PIN_NR_TEMP_0 MARLIN_PIN_NR_0 //ADC

#define PIN_TABLE(F)                                                                                            \
    F(InputPin, zMin, BUDDY_PIN(Z_MIN), IMode::IT_faling COMMA Pull::up)                                        \
    F(InputPin, xDiag, BUDDY_PIN(X_DIAG), IMode::input COMMA Pull::none)                                        \
    F(InputPin, yDiag, BUDDY_PIN(Y_DIAG), IMode::input COMMA Pull::none)                                        \
    F(InputPin, zDiag, BUDDY_PIN(Z_DIAG), IMode::input COMMA Pull::none)                                        \
    F(InputPin, e0Diag, BUDDY_PIN(E0_DIAG), IMode::input COMMA Pull::none)                                      \
    F(OutputPin, xEnable, BUDDY_PIN(X_ENA), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low)           \
    F(OutputPin, yEnable, BUDDY_PIN(Y_ENA), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low)           \
    F(OutputPin, zEnable, BUDDY_PIN(Z_ENA), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low)           \
    F(OutputPin, e0Enable, BUDDY_PIN(E0_ENA), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low)         \
    F(OutputPin, xStep, BUDDY_PIN(X_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)             \
    F(OutputPin, yStep, BUDDY_PIN(Y_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)             \
    F(OutputPin, zStep, BUDDY_PIN(Z_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)             \
    F(OutputPin, e0Step, BUDDY_PIN(E0_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)           \
    F(OutputPin, xDir, BUDDY_PIN(X_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)               \
    F(OutputPin, yDir, BUDDY_PIN(Y_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)               \
    F(OutputPin, zDir, BUDDY_PIN(Z_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)               \
    F(OutputPin, e0Dir, BUDDY_PIN(E0_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)             \
    F(InputPin, fastBoot, IoPort::C COMMA IoPin::p7, IMode::input COMMA Pull::up)                               \
    F(InputPin, fSensor, IoPort::B COMMA IoPin::p4, IMode::input COMMA Pull::up)                                \
    F(OutputPin, fan0pwm, IoPort::E COMMA IoPin::p11, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::high) \
    F(OutputPin, fan1pwm, IoPort::E COMMA IoPin::p9, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::high)  \
    F(InputPin, fan0tach, IoPort::E COMMA IoPin::p10, IMode::input COMMA Pull::up)                              \
    F(InputPin, fan1tach, IoPort::E COMMA IoPin::p14, IMode::input COMMA Pull::up)                              \
    F(InputPin, jogWheelEN1, IoPort::E COMMA IoPin::p15, IMode::input COMMA Pull::up)                           \
    F(InputPin, jogWheelEN2, IoPort::E COMMA IoPin::p13, IMode::input COMMA Pull::up)                           \
    F(InputPin, jogWheelENC, IoPort::E COMMA IoPin::p12, IMode::input COMMA Pull::up)                           \
    F(OutputPin, cs, IoPort::C COMMA IoPin::p9, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::high)      \
    F(OutputPin, rs, IoPort::D COMMA IoPin::p11, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::high)     \
    F(OutputInputPin, rst, IoPort::C COMMA IoPin::p8, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low)

PIN_TABLE(DECLARE_PINS)
