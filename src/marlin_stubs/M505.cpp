#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"

#include <stdio.h>
#include "PrusaGcodeSuite.hpp"

/**
 * @brief M505 is deprecated since old_eeprom -> config_store migration. The file is currently not compiled/linked and serves as a reminder that in case M505 was readded, it should be readded with the same meaning (ie setting a configuration in the printer).
 *
 */

void PrusaGcodeSuite::M505() {
    SERIAL_ERROR_MSG("M505 was deprecated");
    // char var_name[32];
    // char var_value[64];
    // if (sscanf(parser.string_arg, "%s %s", var_name, var_value) == 2) {
    // enum eevar_id var_id;
    // if (!eeprom_find_var_by_name(var_name, &var_id)) {
    //     SERIAL_ERROR_MSG("Variable not found");
    //     return;
    // }

    // variant8_t var = eeprom_var_parse(var_id, var_value);
    // if (variant8_get_type(var) == VARIANT8_ERROR) {
    //     SERIAL_ERROR_MSG("Failed to parse the variable value");
    //     return;
    // }

    // SERIAL_ECHO_START();
    // SERIAL_ECHOPAIR_F("Updating variable ", var_name);
    // SERIAL_ECHOLNPAIR_F(" to value", var_value);
    // eeprom_set_var(var_id, var);
    // } else {
    //     SERIAL_ERROR_MSG("Invalid format");
    // }
}
