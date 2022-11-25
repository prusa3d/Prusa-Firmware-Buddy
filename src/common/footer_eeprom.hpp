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
static constexpr size_t count_of_trailing_ones = 3;
static const size_t value_bit_size = 5; // 32 different items should be enough
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
    uint32_t ret = uint32_t(rec[0]) << count_of_trailing_ones;
    for (size_t i = 1; i < count; ++i) {
        ret |= uint32_t(rec[i]) << ((value_bit_size * i) + count_of_trailing_ones);
    }
    //adding trailing ones to force default footer settings in FW version < 4.4.0 and using fixed size of footer item
    uint32_t trailing_ones = pow(2, count_of_trailing_ones) - 1;
    ret |= trailing_ones;
    return ret;
}

/**
 * @brief decodes footer setting from uint32_t with minimal size one item
 * Does not check if we have trailing zeros
 *
 * @param encoded
 * @param min_bit_size number of bits needed to encode one item
 * @return  record
 */
record DecodeWithSize(uint32_t encoded, size_t min_bit_size);
/**
 * @brief decodes footer setting from uint32_t
 *
 * @param encoded
 * @return  record
 */
record Decode(uint32_t encoded);

/**
 * Converts footer setting between old and current eeprom
 * @param encoded content of old eeprom
 * @param number_of_items_in_old_footer_eeprom number of possible items in old eeprom
 * @return encoded items in current encoding
 */
uint32_t ConvertFromOldEeprom(uint32_t encoded, size_t number_of_items_in_old_footer_eeprom);

}
