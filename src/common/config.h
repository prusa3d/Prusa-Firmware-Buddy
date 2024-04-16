// config.h - main configuration file
#pragma once

#include "printers.h"
#include <stdint.h>
#include <device/board.h>
#include "MarlinPin.h"

//--------------------------------------
// BUDDY_ENABLE_ETHERNET configuration
#include <option/buddy_enable_wui.h>
#if BUDDY_ENABLE_WUI()
    #define BUDDY_ENABLE_ETHERNET
#endif // BUDDY_ENABLE_WUI()

// marlin api config
enum {
    MARLIN_MAX_CLIENTS = 6, // maximum number of clients registered in same time
    MARLIN_MAX_REQUEST = 110, // maximum request length in chars
};

// default string used as LAN hostname
#if PRINTER_IS_PRUSA_MK4
    #define LAN_HOSTNAME_DEF "PrusaMK4"
#elif PRINTER_IS_PRUSA_MK3_5
    #define LAN_HOSTNAME_DEF "PrusaMK4"
#elif PRINTER_IS_PRUSA_XL
    #define LAN_HOSTNAME_DEF "PrusaXL"
#elif PRINTER_IS_PRUSA_iX
    #define LAN_HOSTNAME_DEF "Prusa_iX"
#else
    #define LAN_HOSTNAME_DEF "PrusaMINI"
#endif

#if defined(_DEBUG)
    #define BUDDY_ENABLE_DFU_ENTRY
#endif

// Enabled Z calibration (MK3, MK4, XL)
#if (PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_XL)
    #define WIZARD_Z_CALIBRATION
#endif

// CRC32 config - use hardware CRC32 with RTOS
#define CRC32_USE_HW
#define CRC32_USE_RTOS
