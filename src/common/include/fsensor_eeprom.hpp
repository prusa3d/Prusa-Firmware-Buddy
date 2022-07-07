/**
 * @file fsensor_eeprom.hpp
 * @author Radek Vana
 * @brief api header, meant to be switched with other in unit tests
 * @date 2021-02-11
 */

#pragma once

#include "eeprom.h"

class FSensorEEPROM {
public:
    inline static void Set() { eeprom_set_bool(EEVAR_FSENSOR_ENABLED, true); }
    inline static void Clr() { eeprom_set_bool(EEVAR_FSENSOR_ENABLED, false); }
    inline static bool Get() { return eeprom_get_bool(EEVAR_FSENSOR_ENABLED); }
};
