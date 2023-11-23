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
};

#if defined(USE_ST7789)
    #define SPLASHSCREEN_PROGRESSBAR_X 16
    #define SPLASHSCREEN_PROGRESSBAR_Y 148
    #define SPLASHSCREEN_PROGRESSBAR_W 206
    #define SPLASHSCREEN_PROGRESSBAR_H 12
    #define SPLASHSCREEN_VERSION_Y     165
#elif defined(USE_ILI9488)
    #define SPLASHSCREEN_PROGRESSBAR_X 100
    #define SPLASHSCREEN_PROGRESSBAR_Y 165
    #define SPLASHSCREEN_PROGRESSBAR_W 280
    #define SPLASHSCREEN_PROGRESSBAR_H 12
    #define SPLASHSCREEN_VERSION_Y     185
#endif

struct __attribute__((packed)) version_t {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
};
