//config.h - main configuration file
#ifndef _CONFIG_H
#define _CONFIG_H

#include "printers.h"
#include <avr/pgmspace.h>
#include "config_a3ides2209_02.h"

//--------------------------------------
//BUDDY_ENABLE_ETHERNET configuration
#ifdef BUDDY_ENABLE_WUI
    #define BUDDY_ENABLE_ETHERNET
#endif //BUDDY_ENABLE_WUI
//--------------------------------------
//LCDSIM configuration

//defined as external
#ifdef LCDSIM
    #undef BUDDY_ENABLE_ETHERNET
    #define LCDSIM_COLS 20
    #define LCDSIM_ROWS 4
#endif //LCDSIM

//marlin api config
#define MARLIN_MAX_CLIENTS 3   // maximum number of clients registered in same time
#define MARLIN_MAX_REQUEST 100 // maximum request length in chars

//display PSOD instead of BSOD
//#define PSOD_BSOD

//PID calibration service screen
#define PIDCALIBRATION

//CRC32 config - use hardware CRC32 with RTOS
#define CRC32_USE_HW
#define CRC32_USE_RTOS

//guiconfig.h included with config
#include "guiconfig.h"

//resource.h included with config
#include "resource.h"

#endif //_CONFIG_H
