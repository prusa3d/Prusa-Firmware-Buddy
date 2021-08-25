/**
 * @file footer_eeprom.hpp
 * @author Radek Vana
 * @brief Definition of data stored in eeprom
 * @date 2021-05-20
 */

#pragma once
#include "footer_def.hpp"
#include <cmath>
#include "changed.hpp"

namespace footer::eeprom {

static constexpr size_t count = FOOTER_ITEMS_PER_LINE__;
static const size_t value_bit_size = floor(log2(size_t(items::count_))) + 1; // minimum bits needed to store variable
static_assert(count * value_bit_size <= 32, "Encoded eeprom record is too big");

/**
 * @brief On first call load footer settings from eeprom and store it than return stored value
 *        next calls just return stored value
 * @return record
 */
record Load();

/**
 * @brief save footer settings to eeprom
 *        and update local variable
 * @param rec
 * @return changed_t::yes - value chaged - was stored
 * @return changed_t::no - value already stored in eeprom - was not stored
 */
changed_t Store(record rec);

/**
 * @brief store single footer item ID to eeprom
 *        and update local variable
 * @param item footer item ID
 * @param index index in footer record (array)
 * @return true success, value changed
 * @return false failed (index >= count) or value already stored in eeprom
 */
bool Set(items item, size_t index);

/**
 * @brief On first call load draw config from eeprom and store it than return stored value
 *        next calls just return stored value
 * @return ItemDrawCnf
 */
ItemDrawCnf LoadItemDrawCnf();

/**
 * @brief save footer draw config to eeprom
 *        and update local variable
 * @param cnf
 */
changed_t Set(ItemDrawCnf cnf);

/**
 * @brief save footer draw type to eeprom
 *        and update local variable
 * @param type
 */
changed_t Set(ItemDrawType type);

/**
 * @brief save footer draw zero option to eeprom
 *        and update local variable
 * @param zero
 */
changed_t Set(draw_zero_t zero);

/**
 * @brief save footer centerNAndFewer option to eeprom
 *        and update local variable
 * @param zero
 */
changed_t SetCenterNAndFewer(uint8_t centerNAndFewer);

/**
 * @brief Get the Item Draw Type object
 *
 * @return ItemDrawType
 */
ItemDrawType GetItemDrawType();

/**
 * @brief Get the Item Draw Zero object
 *
 * @return draw_zero_t
 */
draw_zero_t GetItemDrawZero();

/**
 * @brief Get the Center N And Fewer value
 *
 * @return uint8_t
 */
uint8_t GetCenterNAndFewer();

/**
 * @brief encodes footer setting to uint32_t
 *
 * @param rec footer setting to encode
 * @return constexpr uint32_t
 */
constexpr uint32_t Encode(record rec) {
    uint32_t ret = uint32_t(rec[0]);
    for (size_t i = 1; i < count; ++i) {
        ret |= uint32_t(rec[i]) << (value_bit_size * i);
    }
    return ret;
}

/**
 * @brief decodes footer setting from uint32_t
 *
 * @param encoded
 * @return constexpr record
 */
constexpr record Decode(uint32_t encoded) {
    constexpr uint32_t mask = (uint32_t(1) << (value_bit_size)) - 1;
    record ret = { {} };

    for (size_t i = 0; i < count; ++i) {
        uint32_t decoded = encoded & mask;
        if (decoded > size_t(items::count_))
            return footer::DefaultItems; // data corrupted, return default setting
        ret[i] = items(decoded);
        encoded >>= value_bit_size;
    }
    return ret;
}

}
