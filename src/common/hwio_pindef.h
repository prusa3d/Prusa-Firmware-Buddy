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
 * MACRO_FUNCTION(buddy::hw::OutputPin, xStep, BUDDY_PIN(X_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler)
 * end note
 *
 * file "lib/Marlin/Marlin/src/pins/stm32/pins_BUDDY_2209_02.h" as pins
 * note right
 * ~#define X_STEP_PIN             MARLIN_PIN(X_STEP)
 * end note
 *
 * file "src/common/hwio_buddy_2209_02.cpp" as hwio
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
#include "MarlinPin.hpp"
#include "../../lib/Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/endstop_ISR.h"
#include <type_traits>

#if (!defined(PRINTER_PRUSA_MINI))
    #error "Some printer type not defined."
#endif
#if 0
#else

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
        #define PIN_TABLE_BOARD_SPECIFIC(MACRO_FUNCTION)                                                                                                                                           \
            MACRO_FUNCTION(buddy::hw::InterruptPin, zMin, BUDDY_PIN(Z_MIN), IMode::IT_rising_falling COMMA Pull::up COMMA 15 COMMA 0, endstop_ISR)                                                 \
            MACRO_FUNCTION(buddy::hw::OutputPin, yEnable, BUDDY_PIN(Y_ENA), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler)                                        \
            MACRO_FUNCTION(buddy::hw::OutputPin, displayCs, buddy::hw::IoPort::C COMMA buddy::hw::IoPin::p9, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler)      \
            MACRO_FUNCTION(buddy::hw::OutputPin, displayRs, buddy::hw::IoPort::D COMMA buddy::hw::IoPin::p11, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler)     \
            MACRO_FUNCTION(buddy::hw::OutputInputPin, displayRst, buddy::hw::IoPort::C COMMA buddy::hw::IoPin::p8, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
            MACRO_FUNCTION(buddy::hw::InputPin, jogWheelEN1, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p15, IMode::input COMMA Pull::up, buddy::hw::noHandler)                                  \
            MACRO_FUNCTION(buddy::hw::InputPin, jogWheelEN2, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p13, IMode::input COMMA Pull::up, buddy::hw::noHandler)                                  \
            MACRO_FUNCTION(buddy::hw::InputPin, fSensor, buddy::hw::IoPort::B COMMA buddy::hw::IoPin::p4, IMode::input COMMA Pull::up, buddy::hw::noHandler)                                       \
            MACRO_FUNCTION(buddy::hw::OutputPin, hsUSBEnable, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p5, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler)      \
            MACRO_FUNCTION(buddy::hw::InputPin, jogWheelENC, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p12, IMode::input COMMA Pull::up, buddy::hw::noHandler)                                  \
            MACRO_FUNCTION(buddy::hw::InputPin, fanPrintTach, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p10, IMode::input COMMA Pull::up, buddy::hw::noHandler)                                 \
            MACRO_FUNCTION(buddy::hw::InputPin, fanHeatBreakTach, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p14, IMode::input COMMA Pull::up, buddy::hw::noHandler)                             \
            MACRO_FUNCTION(buddy::hw::OutputPin, extFlashCs, buddy::hw::IoPort::D COMMA buddy::hw::IoPin::p7, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler)
    #else
        #error "Unknown board."
    #endif
/**
 * @brief Define @p PIN_TABLE macro containing all physical pins used in project.
 *
 * When defining @p PIN_TABLE use COMMA macro to separate parameters inside sections PORTPIN and PARAMETERS,
 * use ordinary comma (,) to separate sections (TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER).
 *
 * Physical pins accessed by Marlin needs to be repeated here, use BUDDY_PIN() macro to convert Marlin pin into Buddy Pin.
 * This doesn't apply for virtual pins (MARLIN_PORT_V).
 *
 * @par Sections:
 * @n @p TYPE pin type e.g. InputPin, OutputPin, OutputInputPin, InterruptPin,  ...
 * @n @p NAME Name used to access pin. E.g. fastBoot, later accessed as e.g. fastboot.read()
 * @n @p PORTPIN Physical location of pin. E.g. IoPort::C COMMA IoPin::p7 or BUDDY_PIN(E0_DIR) for pin defined for Marlin earlier.
 * @n @p PARAMETERS Parameters passed to pin constructor. Number and type of parameters varies between Pins @p TYPE
 * @n @p INTERRUPT_HANDLER Name of the function to be called on pin interrupt with no non-default parameters.
 *           Return value is ignored. Use buddy::hw::noHandler if this is not InterruptPin.
 *           Symbol used here must be visible in ExtInterruptHandler.cpp. So include needed header file in
 *           ExtInterruptHandler.cpp.
 *
 *           @warning @p INTERRUPT_HANDLER might be called more often than expected.
 *           There are less interrupt lines than number of all pins so one line can be shared between more pins.
 *           If the action done by the interrupt handler can not be arbitrarily repeated it is handler responsibility
 *           to check trigger condition itself. (Or read the note.)
 *
 *           @note Unique interrupt can be easily implemented when needed and allowed by pinout.
 *           E.g. add template parameter enum class Type{Shared, Unique} to InterruptPin class.
 *           And add compile time check to PinsCheck.cpp so none InterruptPin<Type::Unique> in PIN_TABLE,
 *           shares the same pin number with any other InterruptPin.
 *
 * @par Example usage:
 * @code
 * #define PIN_TABLE(MACRO_FUNCTION) \
 *      MACRO_FUNCTION(buddy::hw::OutputPin, e0Dir, BUDDY_PIN(E0_DIR), InitState::reset COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
 *      MACRO_FUNCTION(buddy::hw::InputPin, fastBoot, IoPort::C COMMA IoPin::p7, IMode::input COMMA Pull::up, buddy::hw::noHandler) \
 *      MACRO_FUNCTION(buddy::hw::InterruptPin, yDiag, BUDDY_PIN(Y_DIAG), IMode::IT_rising_falling COMMA Pull::none COMMA 15 COMMA 0, endstops.poll)
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
    #define PIN_TABLE(MACRO_FUNCTION)                                                                                                                                                       \
        PIN_TABLE_BOARD_SPECIFIC(MACRO_FUNCTION)                                                                                                                                            \
        MACRO_FUNCTION(buddy::hw::InterruptPin, xDiag, BUDDY_PIN(X_DIAG), IMode::IT_rising_falling COMMA Pull::none COMMA 15 COMMA 0, endstop_ISR)                                          \
        MACRO_FUNCTION(buddy::hw::InterruptPin, yDiag, BUDDY_PIN(Y_DIAG), IMode::IT_rising_falling COMMA Pull::none COMMA 15 COMMA 0, endstop_ISR)                                          \
        MACRO_FUNCTION(buddy::hw::InterruptPin, zDiag, BUDDY_PIN(Z_DIAG), IMode::IT_rising_falling COMMA Pull::none COMMA 15 COMMA 0, endstop_ISR)                                          \
        MACRO_FUNCTION(buddy::hw::InputPin, e0Diag, BUDDY_PIN(E0_DIAG), IMode::input COMMA Pull::none, buddy::hw::noHandler)                                                                \
        MACRO_FUNCTION(buddy::hw::OutputPin, xEnable, BUDDY_PIN(X_ENA), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler)                                     \
        MACRO_FUNCTION(buddy::hw::OutputPin, zEnable, BUDDY_PIN(Z_ENA), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler)                                     \
        MACRO_FUNCTION(buddy::hw::OutputPin, e0Enable, BUDDY_PIN(E0_ENA), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler)                                   \
        MACRO_FUNCTION(buddy::hw::OutputPin, xStep, BUDDY_PIN(X_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler)                                       \
        MACRO_FUNCTION(buddy::hw::OutputPin, yStep, BUDDY_PIN(Y_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler)                                       \
        MACRO_FUNCTION(buddy::hw::OutputPin, zStep, BUDDY_PIN(Z_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler)                                       \
        MACRO_FUNCTION(buddy::hw::OutputPin, e0Step, BUDDY_PIN(E0_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler)                                     \
        MACRO_FUNCTION(buddy::hw::OutputPin, xDir, BUDDY_PIN(X_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler)                                         \
        MACRO_FUNCTION(buddy::hw::OutputPin, yDir, BUDDY_PIN(Y_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler)                                         \
        MACRO_FUNCTION(buddy::hw::OutputPin, zDir, BUDDY_PIN(Z_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler)                                         \
        MACRO_FUNCTION(buddy::hw::OutputPin, e0Dir, BUDDY_PIN(E0_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler)                                       \
        MACRO_FUNCTION(buddy::hw::OutputPin, fanPrintPwm, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p11, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler) \
        MACRO_FUNCTION(buddy::hw::OutputPin, fanHeatBreakPwm, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p9, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler)
#endif
/**
 * @brief Define @p VIRTUAL_PIN_TABLE macro containing all virtual pins
 *
 * When defining @p VIRTUAL_PIN_TABLE use COMMA macro to separate parameters inside sections PORTPIN and PARAMETERS,
 * use ordinary comma (,) to separate sections (TYPE, READ_FN, ISR_FN, NAME, PORTPIN, PARAMETERS).
 *
 * Virtual pins (MARLIN_PORT_V) accessed by Marlin needs to be repeated here, use BUDDY_PIN() macro to convert Marlin pin into Buddy Pin.
 *
 * @par Sections:
 * @n @p TYPE pin type. At this moment only VirtualInterruptPin is possible option.
 *       @note If implementing new type of the pin, make sure isVirtualInterruptPin() and getInterruptHandler() is adjusted accordingly.
 * @n @p READ_FN Supplied function to read the virtual pin
 * @n @p ISR_FN Supplied function to emulate interrupt routine service call.
 * @n @p NAME Name used to access pin. E.g. zMin, later accessed as e.g. zMin.read()
 * @n @p PORTPIN Virtual location of pin. E.g. BUDDY_PIN(Z_MIN) for virtual pin defined for Marlin earlier.
 * @n @p PARAMETERS Parameters passed to pin constructor.
 *
 * @par Example usage:
 * @code
 * #define VIRTUAL_PIN_TABLE(MACRO_FUNCTION) \
 *          MACRO_FUNCTION(buddy::hw::VirtualInterruptPin, buddy::hw::zMinReadFn, endstop_ISR, zMin, BUDDY_PIN(Z_MIN), IMode::IT_rising_falling)
 *
 * namespace buddy::hw {
 * DECLARE_VIRTUAL_PINS(VIRTUAL_PIN_TABLE)
 * }
 *
 * };
 *
 * @endcode
 *
 */
#define VIRTUAL_PIN_TABLE(MACRO_FUNCTION)

/** @}*/
namespace buddy::hw {
PIN_TABLE(DECLARE_PINS)
VIRTUAL_PIN_TABLE(DECLARE_VIRTUAL_PINS)

/**
 * @brief Convert IoPort and IoPin pair into marlinPin (uint32_t)
 *
 * @param port
 * @param pin
 * @return marlinPin unsigned number used to identify pin inside Marlin
 */
constexpr uint32_t toMarlinPin(IoPort port, IoPin pin) {
    return (MARLIN_PORT_PIN(static_cast<uint32_t>(port), static_cast<uint32_t>(pin)));
}

/**
 * @brief Exist mapping between marlinPin and physical Buddy Pin?
 *
 * @param marlinPin
 * @retval true marlinPin exists in PIN_TABLE
 * @retval false marlinPin does not exists in PIN_TABLE
 */
constexpr bool physicalPinExist(uint32_t marlinPin) {
#define ALL_PHYSICAL_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER) case toMarlinPin(PORTPIN):
    switch (marlinPin) {
        PIN_TABLE(ALL_PHYSICAL_PINS)
        return true;
    default:
        return false;
    }
#undef ALL_PHYSICAL_PINS
}

/**
 * @brief Exist mapping between marlinPin and virtual Buddy Pin?
 *
 * @param marlinPin
 * @retval true marlinPin exists in VIRTUAL_PIN_TABLE
 * @retval false marlinPin does not exists in VIRTUAL_PIN_TABLE
 */
constexpr bool virtualPinExist(uint32_t marlinPin) {
#define ALL_VIRTUAL_PINS(TYPE, READ_FN, ISR_FN, NAME, PORTPIN, PARAMETERS) case toMarlinPin(PORTPIN):
    switch (marlinPin) {
        VIRTUAL_PIN_TABLE(ALL_VIRTUAL_PINS)
        return true;
    default:
        return false;
    }
#undef ALL_VIRTUAL_PINS
}

/**
 * @brief Exist mapping between marlinPin and physical Buddy InterruptPin?
 *
 * @param marlinPin
 * @retval true marlinPin is InterruptPin in PIN_TABLE
 * @retval false marlinPin is not InterruptPin in PIN_TABLE
 */
constexpr bool isInterruptPin(uint32_t marlinPin) {
#define ALL_INTERRUPT_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER) \
    case toMarlinPin(PORTPIN):                                                 \
        return (std::is_same_v<TYPE, InterruptPin>);

    switch (marlinPin) {
        PIN_TABLE(ALL_INTERRUPT_PINS)
    default:
        return false;
    }
#undef ALL_INTERRUPT_PINS
}

/**
 * @brief Exist mapping between marlinPin and VirtualInterruptPin?
 *
 * @param marlinPin
 * @retval true marlinPin is VirtualInterruptPin in VIRTUAL_PIN_TABLE
 * @retval false marlinPin is not VirtualInterruptPin in VIRTUAL_PIN_TABLE
 */
constexpr bool isVirtualInterruptPin(uint32_t marlinPin) {
    return virtualPinExist(marlinPin);
}

} // namespace buddy::hw
