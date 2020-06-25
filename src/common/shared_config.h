//config.h - main configuration file
#ifndef _SHARED_CONFIG_H
#define _SHARED_CONFIG_H

#include "stdint.h"

// bootloader version
#define BOOTLOADER_VERSION_ADDRESS 0x0801FFFA

// EEPROM firmware update flag
#define FW_UPDATE_ENABLE  0xAA
#define FW_UPDATE_DISABLE 0x00

typedef enum {
    BT_APPENDIX_EXISTS = 0, /**< Appendix not broken, signature authentication needed */
	BT_APPENDIX_BROKEN	/**< Appendix broken, no signature authentication needed */
} BT_APPENDIX_STATUS;

// model_specific_flags bit locations from LSB
typedef enum {
	BT_APPENDIX_STATUS_LOC = 0,
	BT_BOOT_FLAG_1,
	BT_BOOT_FLAG_2,
	BT_BOOT_FLAG_3,
	BT_BOOT_FLAG_4,
	BT_BOOT_FLAG_5,
	BT_BOOT_FLAG_6,
	BT_BOOT_FLAG_7
} BT_BOOT_FLAGS_LOC;

#pragma pack(push)
#pragma pack(1)
typedef struct {
    uint8_t fw_update_flag;
    uint8_t boot_flags;
    uint8_t reserved2;
    uint8_t reserved3;
    uint8_t reserved4;
    uint8_t reserved5;
    uint8_t reserved6;
    uint8_t reserved7;
    uint8_t reserved8;
    uint8_t reserved9;
    uint8_t reserved10;
    uint8_t reserved11;
    uint8_t reserved12;
    uint8_t reserved13;
    uint8_t reserved14;
    uint8_t reserved15;
} data_exchange_t;

typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
} version_t;

#pragma pack(pop)

#endif /* _SHARED_CONFIG_H */
