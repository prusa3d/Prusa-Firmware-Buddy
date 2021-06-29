//config.h - main configuration file
#pragma once

#include "printers.h"
#include <avr/pgmspace.h>
#include "config_a3ides2209_02.h"

//--------------------------------------
//BUDDY_ENABLE_ETHERNET configuration
#ifdef BUDDY_ENABLE_WUI
    #define BUDDY_ENABLE_ETHERNET
#endif //BUDDY_ENABLE_WUI
//--------------------------------------
//marlin api config
enum {
    MARLIN_MAX_CLIENTS = 3,    // maximum number of clients registered in same time
    MARLIN_MAX_REQUEST = 100,  // maximum request length in chars
    MARLIN_SERVER_QUEUE = 128, // size of marlin server input character queue (number of characters)
    MARLIN_CLIENT_QUEUE = 16,  // size of marlin client input message queue (number of messages)
};

//display PSOD instead of BSOD
//#define PSOD_BSOD

//CRC32 config - use hardware CRC32 with RTOS
#define CRC32_USE_HW
#define CRC32_USE_RTOS

//guiconfig.h included with config
#include "../guiconfig/guiconfig.h"

//resource.h included with config
#include "resource.h"
