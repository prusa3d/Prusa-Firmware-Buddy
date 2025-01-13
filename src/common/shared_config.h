// config.h - main configuration file
#pragma once

#include "stdint.h"

// bootloader version
static const uint32_t BOOTLOADER_VERSION_ADDRESS = 0x0801FFFA;

// EEPROM firmware update flag
enum class FwAutoUpdate : uint8_t {
    on = 0xAA,
    off = 0x00,
    older = 0x55,
    specified = 0xBB,
    tester_mode_1 = 0xCA,
    tester_mode_2 = 0xCB,
    tester_mode_3 = 0xCC,
};

struct __attribute__((packed)) version_t {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
};
