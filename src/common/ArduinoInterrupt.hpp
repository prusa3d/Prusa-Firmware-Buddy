/**
 * @file
 * @date Nov 12, 2021
 * @author Marek Bel
 */

#pragma once

#include "hwio_pindef.h"
#include <type_traits>
#include <functional>

namespace buddy::hw {

constexpr const char *getInterruptHandler(uint32_t marlinPin) {
#define ALL_INTERRUPT_HANDLERS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER) \
    case toMarlinPin(PORTPIN):                                                     \
        return #INTERRUPT_HANDLER;
#define ALL_VIRTUAL_INTERRUPT_HANDLERS(TYPE, READ_FN, ISR_FN, NAME, PORTPIN, PARAMETERS) \
    case toMarlinPin(PORTPIN):                                                           \
        return #ISR_FN;

    switch (marlinPin) {
        PIN_TABLE(ALL_INTERRUPT_HANDLERS)
        VIRTUAL_PIN_TABLE(ALL_VIRTUAL_INTERRUPT_HANDLERS)
    default:
        return "";
    }
#undef ALL_INTERRUPT_HANDLERS
#undef ALL_VIRTUAL_INTERRUPT_HANDLERS
}

constexpr bool strings_equal(char const *a, char const *b) {
    return *a == *b && (*a == '\0' || strings_equal(a + 1, b + 1));
}

constexpr IMode getInterruptMode(IMode iMode, Pull, uint8_t, uint8_t) {
    return iMode;
}

constexpr IMode getInterruptMode(IMode iMode) {
    return iMode;
}

constexpr IMode getInterruptMode(...) {
    return IMode::input;
}

constexpr IMode getInterruptMode(uint32_t marlinPin) {
#define ALL_INTERRUPT_HANDLERS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)       \
    case toMarlinPin(PORTPIN):                                                           \
        if (std::is_same_v<TYPE, InterruptPin> || std::is_base_of_v<InterruptPin, TYPE>) \
            return getInterruptMode(PARAMETERS);
#define ALL_VIRTUAL_INTERRUPT_HANDLERS(TYPE, READ_FN, ISR_FN, NAME, PORTPIN, PARAMETERS) \
    case toMarlinPin(PORTPIN):                                                           \
        return getInterruptMode(PARAMETERS);

    switch (marlinPin) {
        PIN_TABLE(ALL_INTERRUPT_HANDLERS)
        VIRTUAL_PIN_TABLE(ALL_VIRTUAL_INTERRUPT_HANDLERS)
    default:
        return IMode::input;
    }
#undef ALL_INTERRUPT_HANDLERS
#undef ALL_VIRTUAL_INTERRUPT_HANDLERS
}

} // namespace buddy::hw

#define CHANGE  buddy::hw::IMode::IT_rising_falling
#define FALLING buddy::hw::IMode::IT_falling
#define RISING  buddy::hw::IMode::IT_rising

/**
 * @brief Check that interrupt is already configured by PIN_TABLE the same way as requested by this call
 *
 * This function should be declared in Arduino.h. But this implementation requires inclusion
 * of headers which breaks Marlin compilation when included in Arduino.h
 * @note @p USER_FUNC is checked only by its name. It is not possible to
 *          compare function pointers in compile time as it is not constexpr.
 *          So the check can fail if local function of the same name and different
 *          definition is passed to attachInterrupt as was defined in PIN_TABLE macro.
 */
#define attachInterrupt(MARLIN_PIN, USER_FUNC, I_MODE)                                                                                                             \
    static_assert((buddy::hw::physicalPinExist(MARLIN_PIN) || buddy::hw::virtualPinExist(MARLIN_PIN)), "Pin " #MARLIN_PIN " does not exist.");                     \
    static_assert((buddy::hw::isInterruptPin(MARLIN_PIN) || buddy::hw::isVirtualInterruptPin(MARLIN_PIN)), "Pin " #MARLIN_PIN " is not (Virtual/-)InterruptPin."); \
    static_assert(buddy::hw::strings_equal(buddy::hw::getInterruptHandler(MARLIN_PIN), #USER_FUNC), "Pin " #MARLIN_PIN " does not have " #USER_FUNC " attached."); \
    static_assert(buddy::hw::getInterruptMode(MARLIN_PIN) == I_MODE, "Pin " #MARLIN_PIN " is not configured as " #I_MODE);
