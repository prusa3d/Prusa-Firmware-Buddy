/**
 * @file eeprom_structure.hpp
 * @brief declaration of eeprom structure
 */

#pragma once

#include "eeprom_current.hpp"

// this pragma pack must remain intact, the ordering of EEPROM variables is not alignment-friendly
#pragma pack(push, 1)

struct eeprom_head_t {
    uint16_t VERSION;
    uint16_t FEATURES;
    uint16_t DATASIZE;
    uint16_t FWVERSION;
    uint16_t FWBUILD;
};

// eeprom vars structure (used for defaults, packed - see above pragma)
struct eeprom_vars_t {
    eeprom_head_t head;
    eeprom::current::vars_body_t body;
    uint32_t CRC32;
};

#pragma pack(pop)
