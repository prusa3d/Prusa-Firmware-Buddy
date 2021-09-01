/**
 * @file
 * @author Marek Bel
 *
 * @brief All GPIOs used by firmware are defined here
 *
 * @startuml
 *
 * title Mapping between buddy::Pin and Marlin pin
 *
 * file "src/common/hwio_pindef.h" as hwio_pindef
 * note right
 * ~#define MARLIN_PORT_X_STEP   MARLIN_PORT_D
 * ~#define MARLIN_PIN_NR_X_STEP MARLIN_PIN_NR_1
 *
 * ~#define PIN_TABLE(MACRO_FUNCTION) \
 * MACRO_FUNCTION(buddy::hw::OutputPin, xStep, BUDDY_PIN(X_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)
 * end note
 *
 * file "lib/Marlin/Marlin/src/pins/stm32/pins_BUDDY_2209_02.h" as pins
 * note right
 * ~#define X_STEP_PIN             MARLIN_PIN(X_STEP)
 * end note
 *
 * file "src/common/hwio_BUDDY_2209_02.cpp" as hwio
 * note right
 * digitalWrite()
 * end note
 *
 * component Marlin
 *
 * hwio_pindef --> pins
 * pins --> Marlin
 * Marlin --> hwio
 * hwio_pindef --> hwio
 *
 * @enduml
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

/**
 * @name Define pins to be accessed from Marlin
 *
 *  * Obey naming convention MARLIN_PORT_\<PIN_NAME\> and MARLIN_PIN_NR_\<PIN_NAME\>
 * @par Example
 * @code
 * #define MARLIN_PORT_E0_DIR   MARLIN_PORT_B
 * #define MARLIN_PIN_NR_E0_DIR MARLIN_PIN_NR_8
 * //inside PIN_TABLE
 * MACRO_FUNCTION(buddy::hw::OutputPin, e0Dir, BUDDY_PIN(E0_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low)
 *
 * //inside hwio_\<board>\.cpp (necessary only for virtual pins (MARLIN_PORT_V))
 * case MARLIN_PIN(E0_DIR):
 *
 * //inside Marlin/pins/\<architecture\>/pins_\<board\>.h
 * #define E0_DIR_PIN             MARLIN_PIN(E0_DIR)
 * @endcode
 *
 *  @{
 */

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

    #if BOARD_IS_BUDDY
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
    #endif

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

/** @}*/

/**
 * @name Define all GPIO pins used in firmware
 *
 * @see PIN_TABLE
 * @{
 */

    #if BOARD_IS_BUDDY
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
    #endif
/**
 * @brief Define @p PIN_TABLE macro containing all physical pins used in project.
 *
 * When defining @p PIN_TABLE use COMMA macro to separate parameters inside sections PORTPIN and PARAMETERS,
 * use ordinary comma (,) to separate sections (TYPE, NAME, PORTPIN, PARAMETERS).
 *
 * Physical pins accessed by Marlin needs to be repeated here, use BUDDY_PIN() macro to convert Marlin pin into Buddy Pin.
 * This doesn't apply for virtual pins (MARLIN_PORT_V).
 *
 * @par Sections:
 * @n @p TYPE pin type e.g. InputPin, OutputPin, OutputInputPin, ...
 * @n @p NAME Name used to access pin. E.g. fastBoot, later accessed as e.g. fastboot.read()
 * @n @p PORTPIN Physical location of pin. E.g. IoPort::C COMMA IoPin::p7 or BUDDY_PIN(E0_DIR) for pin defined for Marlin earlier.
 * @n @p PARAMETERS Parameters passed to pin constructor. Number and type of parameters varies between Pins @p TYPE
 *
 * @par Example usage:
 * @code
 * #define PIN_TABLE(MACRO_FUNCTION) \
 *      MACRO_FUNCTION(buddy::hw::OutputPin, e0Dir, BUDDY_PIN(E0_DIR), InitState::reset COMMA OMode::pushPull COMMA OSpeed::low) \
 *      MACRO_FUNCTION(buddy::hw::InputPin, fastBoot, IoPort::C COMMA IoPin::p7, IMode::input COMMA Pull::up)
 *
 * namespace buddy::hw {
 * DECLARE_PINS(PIN_TABLE)
 * }
 *
 * CONFIGURE_PINS(PIN_TABLE)
 *
 * constexpr PinChecker pinsToCheck[] = {
 *   PINS_TO_CHECK(PIN_TABLE)
 * };
 *
 * @endcode
 *
 */
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

/** @}*/
namespace buddy::hw {
PIN_TABLE(DECLARE_PINS)
}
