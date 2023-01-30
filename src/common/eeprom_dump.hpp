/**
 * @file eeprom_dump.hpp
 * @brief dumps eeprom to xFlash
 */

#pragma once

#include "variant8.h"
#include <cstddef>
#include <cstdint>

// cannot include it - marlin dependency, fw declaration will have to do
class eeprom_vars_t;

namespace eeprom {
// crc error during initialization
bool dump_init_crc_err();

// this could be caused probably only by memory corruption
bool dump_wrong_id__var_known(int enum_eevar_id, variant8_t var);
bool dump_wrong_id__var_unknown(int enum_eevar_id);
bool dump_variant_mismatch(int enum_eevar_id, variant8_t var);

/**
 * @brief dump error during write
 * internally reads eeprom again
 * it might happen that we read it without or with different error
 *
 * @param ram_data          current eeprom ram mirror
 * @param iteration         some data are to big to read whole .. number of read cycle, -1 == crc error
 * @param var_id            enum eevar_id
 * @param iteration_buff    data which caused an error, in case iteration == -1, it contains read crc
 * @return true
 * @return false
 */
bool dump_write_err(const eeprom_vars_t &ram_data, int iteration, int var_id, const char *iteration_buff, size_t iteration_buff_sz);

size_t count_of_dumps();
size_t erase_all_dumps();
};
