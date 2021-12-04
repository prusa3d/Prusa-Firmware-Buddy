#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"

#include <stdio.h>
#include "eeprom.h"
#include "eeprom_loadsave.h"
#include "PrusaGcodeSuite.hpp"

void PrusaGcodeSuite::M505() {
    char var_name[32];
    char var_value[64];

    if (sscanf(parser.string_arg, "%s %s", var_name, var_value) == 2) {
        enum eevar_id var_id;
        if (!eeprom_find_var_by_name(var_name, &var_id)) {
            SERIAL_ERROR_MSG("Variable not found");
            return;
        }

        variant8_t var = eeprom_var_parse(var_id, var_value);
        if (variant8_get_type(var) == VARIANT8_ERROR) {
            SERIAL_ERROR_MSG("Failed to parse the variable value");
            return;
        }

        SERIAL_ECHO_START();
        SERIAL_ECHOPAIR_F("Updating variable ", var_name);
        SERIAL_ECHOLNPAIR_F(" to value", var_value);
        eeprom_set_var(var_id, var);
    } else {
        SERIAL_ERROR_MSG("Invalid format");
    }
}
