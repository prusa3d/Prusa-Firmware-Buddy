/**
 * @file
 * @date May 12, 2021
 * @author Marek Bel
 */
#include "MarlinPin.hpp"
#include "hwio_pindef.h"
#include <stdint.h>
#include <type_traits>

namespace buddy::hw {

/**
 * @brief Exist mapping between marlinPin and physical Buddy OutputPin?
 *
 * @param marlinPin
 * @retval true marlinPin is OutputPin in PIN_TABLE
 * @retval false marlinPin is not OutputPin in PIN_TABLE
 */
bool isOutputPin(uint32_t marlinPin) {
#define ALL_OUTPUT_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER) \
    case toMarlinPin(PORTPIN):                                              \
        return (std::is_same_v<TYPE, OutputPin>);

    switch (marlinPin) {
        PIN_TABLE(ALL_OUTPUT_PINS)
    default:
        return false;
    }
#undef ALL_OUTPUT_PINS
}

} // namespace buddy::hw
