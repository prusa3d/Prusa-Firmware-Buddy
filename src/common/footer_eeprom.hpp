/**
 * @file footer_eeprom.hpp
 * @author Radek Vana
 * @brief Definition of data stored in eeprom
 * @date 2021-05-20
 */

#pragma once
#include "footer_def.hpp"
#include "changed.hpp"
#include <printers.h>

namespace footer::eeprom {
/**
 * @brief Fetches footer item values from eeprom and returns them in a Record array
 *
 */
Record stored_settings_as_record();

/**
 * @brief On first call load draw config from eeprom and store it than return stored value
 *        next calls just return stored value
 * @return ItemDrawCnf
 */
ItemDrawCnf load_item_draw_cnf();

/**
 * @brief save footer draw config to eeprom
 *        and update local variable
 * @param cnf
 */
changed_t set(ItemDrawCnf cnf);

/**
 * @brief save footer draw type to eeprom
 *        and update local variable
 * @param type
 */
changed_t set(ItemDrawType type);

/**
 * @brief save footer draw zero option to eeprom
 *        and update local variable
 * @param zero
 */
changed_t set(draw_zero_t zero);

/**
 * @brief save footer centerNAndFewer option to eeprom
 *        and update local variable
 * @param zero
 */
changed_t set_center_n_and_fewer(uint8_t center_n_and_fewer);

/**
 * @brief Get the Item Draw Type object
 *
 * @return ItemDrawType
 */
ItemDrawType get_item_draw_type();

/**
 * @brief Get the Item Draw Zero object
 *
 * @return draw_zero_t
 */
draw_zero_t get_item_draw_zero();

/**
 * @brief Get the Center N And Fewer value
 *
 * @return uint8_t
 */
uint8_t get_center_n_and_fewer();

#if PRINTER_IS_PRUSA_MINI() || PRINTER_IS_PRUSA_XL()
Record decode_from_old_eeprom_v22(uint32_t encoded);
#endif

} // namespace footer::eeprom
