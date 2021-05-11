/**
 * @file
 * @author Marek Bel
 *
 */
#pragma once
#include "printers.h"
#include "board.h"

#if (!defined(PRINTER_PRUSA_MINI))
    #error "Some printer type not defined."
#endif
#if 0
#else
    #include "MarlinPin.hpp"

    #define MARLIN_PORT_TEMP_BOARD   MARLIN_PORT_V
    #define MARLIN_PIN_NR_TEMP_BOARD MARLIN_PIN_NR_0

    #define MARLIN_PORT_X_DIAG   MARLIN_PORT_E
    #define MARLIN_PIN_NR_X_DIAG MARLIN_PIN_NR_2

    #define MARLIN_PORT_Y_DIAG   MARLIN_PORT_E
    #define MARLIN_PIN_NR_Y_DIAG MARLIN_PIN_NR_1

    #define MARLIN_PORT_X_STEP   MARLIN_PORT_D
    #define MARLIN_PIN_NR_X_STEP MARLIN_PIN_NR_1

    #define MARLIN_PORT_X_DIR   MARLIN_PORT_D
    #define MARLIN_PIN_NR_X_DIR MARLIN_PIN_NR_0

    #define MARLIN_PORT_Y_STEP   MARLIN_PORT_D
    #define MARLIN_PIN_NR_Y_STEP MARLIN_PIN_NR_13

    #define MARLIN_PORT_Y_DIR   MARLIN_PORT_D
    #define MARLIN_PIN_NR_Y_DIR MARLIN_PIN_NR_12

    #if (BOARD_TYPE == BUDDY_BOARD)
        #define MARLIN_PORT_Z_MIN     MARLIN_PORT_A
        #define MARLIN_PIN_NR_Z_MIN   MARLIN_PIN_NR_8
        #define MARLIN_PORT_Z_DIR     MARLIN_PORT_D
        #define MARLIN_PIN_NR_Z_DIR   MARLIN_PIN_NR_15
        #define MARLIN_PORT_Z_STEP    MARLIN_PORT_D
        #define MARLIN_PIN_NR_Z_STEP  MARLIN_PIN_NR_4
        #define MARLIN_PORT_Z_ENA     MARLIN_PORT_D
        #define MARLIN_PIN_NR_Z_ENA   MARLIN_PIN_NR_2
        #define MARLIN_PORT_Z_DIAG    MARLIN_PORT_E
        #define MARLIN_PIN_NR_Z_DIAG  MARLIN_PIN_NR_3
        #define MARLIN_PORT_E0_DIR    MARLIN_PORT_D
        #define MARLIN_PIN_NR_E0_DIR  MARLIN_PIN_NR_8
        #define MARLIN_PORT_E0_STEP   MARLIN_PORT_D
        #define MARLIN_PIN_NR_E0_STEP MARLIN_PIN_NR_9
        #define MARLIN_PORT_E0_ENA    MARLIN_PORT_D
        #define MARLIN_PIN_NR_E0_ENA  MARLIN_PIN_NR_10
        #define MARLIN_PORT_E0_DIAG   MARLIN_PORT_A
        #define MARLIN_PIN_NR_E0_DIAG MARLIN_PIN_NR_15
        #define MARLIN_PORT_THERM2    MARLIN_PORT_A
        #define MARLIN_PIN_NR_THERM2  MARLIN_PIN_NR_5 //ADC
        #define MARLIN_PORT_Y_ENA     MARLIN_PORT_D
        #define MARLIN_PIN_NR_Y_ENA   MARLIN_PIN_NR_14
        #define MARLIN_PORT_X_ENA     MARLIN_PORT_D
        #define MARLIN_PIN_NR_X_ENA   MARLIN_PIN_NR_3
    #else
        #error "Unknown board."
    #endif //(BOARD_TYPE == BUDDY_BOARD)

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

    #define MARLIN_PORT_TEMP_HEATBREAK   MARLIN_PORT_A
    #define MARLIN_PIN_NR_TEMP_HEATBREAK MARLIN_PIN_NR_6 //ADC

    #define MARLIN_PORT_TEMP_0   MARLIN_PORT_C
    #define MARLIN_PIN_NR_TEMP_0 MARLIN_PIN_NR_0 //ADC

    #if (BOARD_TYPE == BUDDY_BOARD)
        #define PIN_TABLE_BOARD_SPECIFIC(MACRO_FUNCTION)                                                                                                                     \
            MACRO_FUNCTION(buddy::hw::InputPin, zMin, BUDDY_PIN(Z_MIN), IMode::IT_faling COMMA Pull::up)                                                                     \
            MACRO_FUNCTION(buddy::hw::OutputPin, yEnable, BUDDY_PIN(Y_ENA), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low)                                        \
            MACRO_FUNCTION(buddy::hw::OutputPin, displayCs, buddy::hw::IoPort::C COMMA buddy::hw::IoPin::p9, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::high)      \
            MACRO_FUNCTION(buddy::hw::OutputPin, displayRs, buddy::hw::IoPort::D COMMA buddy::hw::IoPin::p11, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::high)     \
            MACRO_FUNCTION(buddy::hw::OutputInputPin, displayRst, buddy::hw::IoPort::C COMMA buddy::hw::IoPin::p8, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low) \
            MACRO_FUNCTION(buddy::hw::InputPin, jogWheelEN1, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p15, IMode::input COMMA Pull::up)                                  \
            MACRO_FUNCTION(buddy::hw::InputPin, jogWheelEN2, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p13, IMode::input COMMA Pull::up)
    #else
        #error "Unknown board."
    #endif // #if (BOARD_TYPE == BUDDY_BOARD)

    #define PIN_TABLE(MACRO_FUNCTION)                                                                                                                             \
        PIN_TABLE_BOARD_SPECIFIC(MACRO_FUNCTION)                                                                                                                  \
        MACRO_FUNCTION(buddy::hw::InputPin, xDiag, BUDDY_PIN(X_DIAG), IMode::input COMMA Pull::none)                                                              \
        MACRO_FUNCTION(buddy::hw::InputPin, yDiag, BUDDY_PIN(Y_DIAG), IMode::input COMMA Pull::none)                                                              \
        MACRO_FUNCTION(buddy::hw::InputPin, zDiag, BUDDY_PIN(Z_DIAG), IMode::input COMMA Pull::none)                                                              \
        MACRO_FUNCTION(buddy::hw::InputPin, e0Diag, BUDDY_PIN(E0_DIAG), IMode::input COMMA Pull::none)                                                            \
        MACRO_FUNCTION(buddy::hw::OutputPin, xEnable, BUDDY_PIN(X_ENA), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low)                                 \
        MACRO_FUNCTION(buddy::hw::OutputPin, zEnable, BUDDY_PIN(Z_ENA), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low)                                 \
        MACRO_FUNCTION(buddy::hw::OutputPin, e0Enable, BUDDY_PIN(E0_ENA), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low)                               \
        MACRO_FUNCTION(buddy::hw::OutputPin, xStep, BUDDY_PIN(X_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)                                   \
        MACRO_FUNCTION(buddy::hw::OutputPin, yStep, BUDDY_PIN(Y_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)                                   \
        MACRO_FUNCTION(buddy::hw::OutputPin, zStep, BUDDY_PIN(Z_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)                                   \
        MACRO_FUNCTION(buddy::hw::OutputPin, e0Step, BUDDY_PIN(E0_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)                                 \
        MACRO_FUNCTION(buddy::hw::OutputPin, xDir, BUDDY_PIN(X_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)                                     \
        MACRO_FUNCTION(buddy::hw::OutputPin, yDir, BUDDY_PIN(Y_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)                                     \
        MACRO_FUNCTION(buddy::hw::OutputPin, zDir, BUDDY_PIN(Z_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)                                     \
        MACRO_FUNCTION(buddy::hw::OutputPin, e0Dir, BUDDY_PIN(E0_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)                                   \
        MACRO_FUNCTION(buddy::hw::InputPin, fastBoot, buddy::hw::IoPort::C COMMA buddy::hw::IoPin::p7, IMode::input COMMA Pull::up)                               \
        MACRO_FUNCTION(buddy::hw::InputPin, fSensor, buddy::hw::IoPort::B COMMA buddy::hw::IoPin::p4, IMode::input COMMA Pull::up)                                \
        MACRO_FUNCTION(buddy::hw::OutputPin, fan0pwm, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p11, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::high) \
        MACRO_FUNCTION(buddy::hw::OutputPin, fan1pwm, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p9, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::high)  \
        MACRO_FUNCTION(buddy::hw::InputPin, fan0tach, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p10, IMode::input COMMA Pull::up)                              \
        MACRO_FUNCTION(buddy::hw::InputPin, fan1tach, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p14, IMode::input COMMA Pull::up)                              \
        MACRO_FUNCTION(buddy::hw::InputPin, jogWheelENC, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p12, IMode::input COMMA Pull::up)
#endif

namespace buddy::hw {
PIN_TABLE(DECLARE_PINS)
}
