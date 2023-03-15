//! @file
#pragma once

#include "config.h"

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "Configuration_MINI.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_XL) && BOARD_IS_XLBUDDY
    #include "Configuration_XL.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_XL) && BOARD_IS_DWARF
    #include "Configuration_XL_Dwarf.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_MK404)
    #include "Configuration_MK404.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_IXL)
    #include "Configuration_iXL.h"
#else
    #error "Unknown PRINTER_TYPE!"
#endif

#ifdef _EXTUI
    #undef RA_CONTROL_PANEL
    #define EXTENSIBLE_UI
    #define NO_LCD_MENUS
#endif
