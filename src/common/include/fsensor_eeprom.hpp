/**
 * @file fsensor_eeprom.hpp
 * @author Radek Vana
 * @brief api header, meant to be switched with other in unit tests
 * @date 2021-02-11
 */

#pragma once

#include <config_store/store_instance.hpp>

class FSensorEEPROM {
public:
    inline static void Set() { config_store().fsensor_enabled.set(true); }
    inline static void Clr() { config_store().fsensor_enabled.set(false); }
    inline static bool Get() { return config_store().fsensor_enabled.get(); }
};
