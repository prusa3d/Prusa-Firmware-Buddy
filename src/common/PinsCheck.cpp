/**
 * @file
 * @date Sep 19, 2020
 * @author Marek Bel
 */

#include "hwio_pindef.h"
#include <iterator>

using buddy::hw::PinChecker;

constexpr PinChecker pinsToCheck[] = {
    PIN_TABLE(PINS_TO_CHECK)
};

constexpr bool has_duplicates(const PinChecker (&array)[std::size(pinsToCheck)]) {
    for (size_t i = 1; i < std::size(array); i++)
        for (size_t j = 0; j < i; j++)
            if ((array[i].getPort() == array[j].getPort()) && (array[i].getPin() == array[j].getPin()))
                return true;
    return false;
}

static_assert(!has_duplicates(pinsToCheck), "Some physical pin defined under two names.");
