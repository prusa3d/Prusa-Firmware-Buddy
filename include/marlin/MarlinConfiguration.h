//Configuration.h
#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#include "config.h"

#if (MOTHERBOARD == 1820)
    //discontinued
    #error "Board not supported!"

    #if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
        #include "Configuration_A3ides_2130_MINI.h"
    #else
        #error "Unknown PRINTER_TYPE!"
    #endif

#elif (MOTHERBOARD == 1821)

    #if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
        #include "Configuration_A3ides_2209_MINI.h"
    #else
        #error "Unknown PRINTER_TYPE!"
    #endif

#elif (MOTHERBOARD == 1823)

    #if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
        #include "Configuration_A3ides_2209_MINI.h"
    #else
        #error "Unknown PRINTER_TYPE!"
    #endif

#else
    #error "Unknown MOTHERBOARD!"
#endif

#ifdef _EXTUI
    #undef RA_CONTROL_PANEL
    #define EXTENSIBLE_UI
    #define NO_LCD_MENUS
#endif

#endif //_CONFIGURATION_H
