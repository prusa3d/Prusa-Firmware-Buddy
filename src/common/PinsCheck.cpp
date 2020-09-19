/**
 * @file
 * @date Sep 19, 2020
 * @author Marek Bel
 */

#include "hwio_pindef.h"
#include <iterator>

struct PinToCheck {
    decltype(OutputPin::m_halPortBase) port;
    decltype(OutputPin::m_halPin) pin;
};
constexpr PinToCheck pinsToCheck[] = {
    PIN_TABLE(PINS_TO_CHECK)
};

constexpr bool has_duplicates(const PinToCheck (&array)[std::size(pinsToCheck)]) {
    for (size_t i = 1; i < std::size(array); i++)
        for (size_t j = 0; j < i; j++)
            if ((array[i].port == array[j].port) && (array[i].pin == array[j].pin))
                return 1;
    return 0;
}

static_assert(!has_duplicates(pinsToCheck), "Some physical pin defined under two names.");
