// config.h - main configuration file
#pragma once

#include "stdint.h"

// bootloader version
static const uint32_t BOOTLOADER_VERSION_ADDRESS = 0x0801FFFA;

// EEPROM firmware update flag
static const uint8_t FW_UPDATE_ENABLE = 0xAA;    // Automatically update newer FW
static const uint8_t FW_UPDATE_OLDER = 0x55;     // Automatically update (even older) FW
static const uint8_t FW_UPDATE_DISABLE = 0x00;   // Do not update automatically
static const uint8_t FW_UPDATE_SPECIFIED = 0xBB; // Automatically update specific FW from USB

// pin PA13 state
static const uint8_t APPENDIX_FLAG_MASK = 0x01;

#pragma pack(push)
#pragma pack(1)

typedef struct {
    uint8_t fw_update_flag;       // On / Off / Older / Specified
    uint8_t model_specific_flags; // ~ "reserved1" originally
    uint8_t fw_signature;
    uint8_t bootloader_valid;     // Initialized by preboot to false; bootloader sets it to true
    char bbf_sfn[13];             // Short file name of specified BBF
    uint8_t reverved17;
    uint8_t reserved18;
    uint8_t reserved19;
    uint8_t reserved20;
    uint8_t reserved21;
    uint8_t reserved22;
    uint8_t reserved23;
    uint8_t reserved24;
    uint8_t reserved25;
    uint8_t reserved26;
    uint8_t reserved27;
    uint8_t reserved28;
    uint8_t reserved29;
    uint8_t reserved30;
    uint8_t reserved31;
} data_exchange_t;

typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
} version_t;

#pragma pack(pop)
