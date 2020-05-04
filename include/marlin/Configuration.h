//! @file
#pragma once

#include "config.h"

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "Configuration_A3ides_2209_MINI.h"
#else
    #error "Unknown PRINTER_TYPE!"
#endif

#ifdef _EXTUI
    #undef RA_CONTROL_PANEL
    #define EXTENSIBLE_UI
    #define NO_LCD_MENUS
#endif
