// config_buddy_2209_02.h - configuration file for 2209 variant (rev02)
#pragma once
#include <stdint.h>

#include <device/board.h>
#include "printers.h"
#include "MarlinPin.h"
#define PRUSA_MARLIN_API

//--------------------------------------
// DBG - debug/trace configuration
#ifdef _DEBUG
    #define DBG_LEVEL 1 // debug level (0..3)
#endif //_DEBUG

//--------------------------------------
// WDT - watchdog timers (IWDG, WWDG)
#ifndef _DEBUG
    #define WDT_IWDG_ENABLED
// #define WDT_WWDG_ENABLED
#endif //_DEBUG

// show filament sensor status in header
// #define DEBUG_FSENSOR_IN_HEADER

// new pause settings
static const uint8_t PAUSE_NOZZLE_TIMEOUT = 45; // nozzle "sleep" after 45s inside paused state
