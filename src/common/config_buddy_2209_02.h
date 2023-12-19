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
