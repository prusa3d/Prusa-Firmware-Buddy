/**
 * @file
 * @date Sep 19, 2020
 * @author Marek Bel
 */

#include "hwio_pindef.h"
#include <iterator>

constexpr Pin pinsToCheck[] = {
    PIN_TABLE(PINS_TO_CHECK)
};

constexpr bool has_duplicates(const Pin (&array)[std::size(pinsToCheck)]) {
    for (size_t i = 1; i < std::size(array); i++)
        for (size_t j = 0; j < i; j++)
            if ((array[i].m_halPortBase == array[j].m_halPortBase) && (array[i].m_halPin == array[j].m_halPin))
                return 1;
    return 0;
}

static_assert(!has_duplicates(pinsToCheck), "Some physical pin defined under two names.");
