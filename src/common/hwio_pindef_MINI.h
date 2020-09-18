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

#if 0 //not used outside of its definition scope
extern InputPin fastBoot;
extern InputPin jogWheelEN1;
extern InputPin jogWheelEN2;
extern InputPin jogWheelENC;
extern OutputPin cs;
extern OutputPin rs;
extern OutputInputPin rst;
extern InputPin fSensor;
#endif

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

#define PIN_TABLE(F)                                                                           \
    F(InputPin, zMin, (BUDDY_PIN(Z_MIN), IMode::IT_faling, Pull::up))                          \
    F(InputPin, xDiag, (BUDDY_PIN(X_DIAG), IMode::input, Pull::none))                          \
    F(InputPin, yDiag, (BUDDY_PIN(Y_DIAG), IMode::input, Pull::none))                          \
    F(InputPin, zDiag, (BUDDY_PIN(Z_DIAG), IMode::input, Pull::none))                          \
    F(InputPin, e0Diag, (BUDDY_PIN(E0_DIAG), IMode::input, Pull::none))                        \
    F(OutputPin, xEnable, (BUDDY_PIN(X_ENA), InitState::set, OMode::pushPull, OSpeed::low))    \
    F(OutputPin, yEnable, (BUDDY_PIN(Y_ENA), InitState::set, OMode::pushPull, OSpeed::low))    \
    F(OutputPin, zEnable, (BUDDY_PIN(Z_ENA), InitState::set, OMode::pushPull, OSpeed::low))    \
    F(OutputPin, e0Enable, (BUDDY_PIN(E0_ENA), InitState::set, OMode::pushPull, OSpeed::low))  \
    F(OutputPin, xStep, (BUDDY_PIN(X_STEP), InitState::reset, OMode::pushPull, OSpeed::low))   \
    F(OutputPin, yStep, (BUDDY_PIN(Y_STEP), InitState::reset, OMode::pushPull, OSpeed::low))   \
    F(OutputPin, zStep, (BUDDY_PIN(Z_STEP), InitState::reset, OMode::pushPull, OSpeed::low))   \
    F(OutputPin, e0Step, (BUDDY_PIN(E0_STEP), InitState::reset, OMode::pushPull, OSpeed::low)) \
    F(OutputPin, xDir, (BUDDY_PIN(X_DIR), InitState::reset, OMode::pushPull, OSpeed::low))     \
    F(OutputPin, yDir, (BUDDY_PIN(Y_DIR), InitState::reset, OMode::pushPull, OSpeed::low))     \
    F(OutputPin, zDir, (BUDDY_PIN(Z_DIR), InitState::reset, OMode::pushPull, OSpeed::low))     \
    F(OutputPin, e0Dir, (BUDDY_PIN(E0_DIR), InitState::reset, OMode::pushPull, OSpeed::low))

PIN_TABLE(DECLARE_PINS)
