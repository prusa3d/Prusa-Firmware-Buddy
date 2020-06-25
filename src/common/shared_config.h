//config.h - main configuration file
#ifndef _SHARED_CONFIG_H
#define _SHARED_CONFIG_H

#include "stdint.h"

// bootloader version
#define BOOTLOADER_VERSION_ADDRESS 0x0801FFFA

typedef enum {
    BT_APPENDIX_EXISTS = 0, /**< Appendix not broken, signature authentication needed */
    BT_APPENDIX_BROKEN /**< Appendix broken, no signature authentication needed */
} BT_APPENDIX_STATUS;

#pragma pack(push)
#pragma pack(1)

typedef enum {
    FW_UPDATE_ENABLE = 0xAA,
    FW_UPDATE_DISABLE = 0xFF,
} fw_update_flag;

typedef struct  {
    fw_update_flag       fw_update_flag;
    bool                 appendix_broken : 1;
    uint8_t              __reserved[14];
} data_exchange_t;

typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
} version_t;

#pragma pack(pop)

#endif /* _SHARED_CONFIG_H */
