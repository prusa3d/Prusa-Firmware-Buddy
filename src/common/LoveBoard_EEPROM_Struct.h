#pragma once

#include <stdint.h>

struct __attribute__((packed)) loveboard_eeprom {
    uint8_t struct_ver;
    uint16_t struct_size;
    uint8_t bom_id;
    uint32_t timestamp;
    uint8_t datamatrix_id[24];
};
