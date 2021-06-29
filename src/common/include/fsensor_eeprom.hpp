/**
 * @file fsensor_eeprom.hpp
 * @author Radek Vana
 * @brief api header, meant to be switched with other in unit tests
 * @date 2021-02-11
 */

#pragma once

namespace {
#include "eeprom.h"
};

class FSensorEEPROM {
public:
    inline static void Set() { eeprom_set_var(EEVAR_FSENSOR_ENABLED, variant8_ui8(1)); }
    inline static void Clr() { eeprom_set_var(EEVAR_FSENSOR_ENABLED, variant8_ui8(0)); }
    inline static bool Get() { return variant_get_ui8(eeprom_get_var(EEVAR_FSENSOR_ENABLED)) ? true : false; }
};
