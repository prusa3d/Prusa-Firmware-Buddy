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

#define PIN_TABLE(F)                                                                                                                                             \
    F(InputPin, zMin, { BUDDY_PIN(Z_MIN) COMMA IMode::IT_faling COMMA Pull::up })                                                                                \
    F(InputPin, xDiag, { BUDDY_PIN(X_DIAG) COMMA IMode::input COMMA Pull::none })                                                                                \
    F(InputPin, yDiag, { BUDDY_PIN(Y_DIAG) COMMA IMode::input COMMA Pull::none })                                                                                \
    F(InputPin, zDiag, { BUDDY_PIN(Z_DIAG) COMMA IMode::input COMMA Pull::none })                                                                                \
    F(InputPin, e0Diag, { BUDDY_PIN(E0_DIAG) COMMA IMode::input COMMA Pull::none })                                                                              \
    F(OutputPin, xEnable, { BUDDY_PIN(X_ENA) COMMA InitState::set COMMA OMode::pushPull COMMA OSpeed::low })                                                     \
    F(OutputPin, yEnable, { BUDDY_PIN(Y_ENA) COMMA InitState::set COMMA OMode::pushPull COMMA OSpeed::low })                                                     \
    F(OutputPin, zEnable, { BUDDY_PIN(Z_ENA) COMMA InitState::set COMMA OMode::pushPull COMMA OSpeed::low })                                                     \
    F(OutputPin, e0Enable, { BUDDY_PIN(E0_ENA) COMMA InitState::set COMMA OMode::pushPull COMMA OSpeed::low })                                                   \
    F(OutputPin, xStep, { BUDDY_PIN(X_STEP) COMMA InitState::reset COMMA OMode::pushPull COMMA OSpeed::low })                                                    \
    F(OutputPin, yStep, { BUDDY_PIN(Y_STEP) COMMA InitState::reset COMMA OMode::pushPull COMMA OSpeed::low })                                                    \
    F(OutputPin, zStep, { BUDDY_PIN(Z_STEP) COMMA InitState::reset COMMA OMode::pushPull COMMA OSpeed::low })                                                    \
    F(OutputPin, e0Step, { BUDDY_PIN(E0_STEP) COMMA InitState::reset COMMA OMode::pushPull COMMA OSpeed::low })                                                  \
    F(OutputPin, xDir, { BUDDY_PIN(X_DIR) COMMA InitState::reset COMMA OMode::pushPull COMMA OSpeed::low })                                                      \
    F(OutputPin, yDir, { BUDDY_PIN(Y_DIR) COMMA InitState::reset COMMA OMode::pushPull COMMA OSpeed::low })                                                      \
    F(OutputPin, zDir, { BUDDY_PIN(Z_DIR) COMMA InitState::reset COMMA OMode::pushPull COMMA OSpeed::low })                                                      \
    F(OutputPin, e0Dir, { BUDDY_PIN(E0_DIR) COMMA InitState::reset COMMA OMode::pushPull COMMA OSpeed::low })                                                    \
    F(InputPin, fastBoot, { Pin::IoPortToHalBase(IoPort::C) COMMA Pin::IoPinToHal(IoPin::p7) COMMA IMode::input COMMA Pull::up })                                \
    F(InputPin, fSensor, { Pin::IoPortToHalBase(IoPort::B) COMMA Pin::IoPinToHal(IoPin::p4) COMMA IMode::input COMMA Pull::up })                                 \
    F(OutputPin, fan0pwm, { Pin::IoPortToHalBase(IoPort::E) COMMA Pin::IoPinToHal(IoPin::p11) COMMA InitState::reset COMMA OMode::pushPull COMMA OSpeed::high }) \
    F(OutputPin, fan1pwm, { Pin::IoPortToHalBase(IoPort::E) COMMA Pin::IoPinToHal(IoPin::p9) COMMA InitState::reset COMMA OMode::pushPull COMMA OSpeed::high })  \
    F(InputPin, fan0tach, { Pin::IoPortToHalBase(IoPort::E) COMMA Pin::IoPinToHal(IoPin::p10) COMMA IMode::input COMMA Pull::up })                               \
    F(InputPin, fan1tach, { Pin::IoPortToHalBase(IoPort::E) COMMA Pin::IoPinToHal(IoPin::p14) COMMA IMode::input COMMA Pull::up })                               \
    F(InputPin, jogWheelEN1, { Pin::IoPortToHalBase(IoPort::E) COMMA Pin::IoPinToHal(IoPin::p15) COMMA IMode::input COMMA Pull::up })                            \
    F(InputPin, jogWheelEN2, { Pin::IoPortToHalBase(IoPort::E) COMMA Pin::IoPinToHal(IoPin::p13) COMMA IMode::input COMMA Pull::up })                            \
    F(InputPin, jogWheelENC, { Pin::IoPortToHalBase(IoPort::E) COMMA Pin::IoPinToHal(IoPin::p12) COMMA IMode::input COMMA Pull::up })                            \
    F(OutputPin, cs, { Pin::IoPortToHalBase(IoPort::C) COMMA Pin::IoPinToHal(IoPin::p9) COMMA InitState::set COMMA OMode::pushPull COMMA OSpeed::high })         \
    F(OutputPin, rs, { Pin::IoPortToHalBase(IoPort::D) COMMA Pin::IoPinToHal(IoPin::p11) COMMA InitState::set COMMA OMode::pushPull COMMA OSpeed::high })        \
    F(OutputInputPin, rst, { Pin::IoPortToHalBase(IoPort::C) COMMA Pin::IoPinToHal(IoPin::p8) COMMA InitState::set COMMA OMode::pushPull COMMA OSpeed::low })

PIN_TABLE(DECLARE_PINS)
