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
 * @brief Convert IoPort and IoPin pair into marlinPin (uint32_t)
 *
 * @param port
 * @param pin
 * @return marlinPin unsigned number used to identify pin inside Marlin
 */
static constexpr uint32_t toMarlinPin(IoPort port, IoPin pin) {
    return (MARLIN_PORT_PIN(static_cast<uint32_t>(port), static_cast<uint32_t>(pin)));
}

/**
 * @brief Exist mapping between marlinPin and physical Buddy Pin?
 *
 * @param marlinPin
 * @retval true marlinPin exists in PIN_TABLE
 * @retval false marlinPin does not exists in PIN_TABLE
 */
bool physicalPinExist(uint32_t marlinPin) {
#define ALL_PHYSICAL_PINS(TYPE, NAME, PORTPIN, PARAMETERS) case toMarlinPin(PORTPIN):
    switch (marlinPin) {
        PIN_TABLE(ALL_PHYSICAL_PINS)
        return true;
    default:
        return false;
    }
#undef ALL_PHYSICAL_PINS
}

/**
 * @brief Exist mapping between marlinPin and physical Buddy OutputPin?
 *
 * @param marlinPin
 * @retval true marlinPin is OutputPin in PIN_TABLE
 * @retval false marlinPin is not OutputPin in PIN_TABLE
 */
bool isOutputPin(uint32_t marlinPin) {
#define ALL_OUTPUT_PINS(TYPE, NAME, PORTPIN, PARAMETERS) \
    case toMarlinPin(PORTPIN):                           \
        return (std::is_same_v<TYPE, OutputPin>);

    switch (marlinPin) {
        PIN_TABLE(ALL_OUTPUT_PINS)
    default:
        return false;
    }
#undef ALL_OUTPUT_PINS
}

} //namespace buddy::hw
