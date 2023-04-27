//! @file
#pragma once

#include <device/board.h>
#include "config.h"

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "Configuration_MINI_adv.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_XL) && BOARD_IS_XLBUDDY
    #include "Configuration_XL_adv.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_XL) && BOARD_IS_DWARF
    #include "Configuration_XL_Dwarf_adv.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_MK4)
    #include "Configuration_MK4_adv.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_IXL)
    #include "Configuration_iXL_adv.h"
#else
    #error "Unknown PRINTER_TYPE!"
#endif
