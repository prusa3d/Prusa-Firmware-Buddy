//! @file
#pragma once

#include "config.h"

#if PRINTER_IS_PRUSA_MINI()
    #include "Configuration_MINI.h"
#elif PRINTER_IS_PRUSA_XL_DEV_KIT()
    #include "Configuration_XL_DEV_KIT.h"
#elif PRINTER_IS_PRUSA_XL() && BOARD_IS_XLBUDDY
    #include "Configuration_XL.h"
#elif PRINTER_IS_PRUSA_XL() && BOARD_IS_DWARF
    #include "Configuration_XL_Dwarf.h"
#elif PRINTER_IS_PRUSA_MK4()
    #include "Configuration_MK4.h"
#elif PRINTER_IS_PRUSA_MK3_5()
    #include "Configuration_MK3.5.h"
#elif PRINTER_IS_PRUSA_iX()
    #include "Configuration_iX.h"
#else
    #error "Unknown PRINTER_TYPE!"
#endif

#ifdef _EXTUI
    #undef RA_CONTROL_PANEL
    #define EXTENSIBLE_UI
    #define NO_LCD_MENUS
#endif
