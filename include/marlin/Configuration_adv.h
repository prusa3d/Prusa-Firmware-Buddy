//! @file
#pragma once

#include <device/board.h>
#include "config.h"

#if PRINTER_IS_PRUSA_MINI()
    #include "Configuration_MINI_adv.h"
#elif PRINTER_IS_PRUSA_XL_DEV_KIT()
    #include "Configuration_XL_DEV_KIT_adv.h"
#elif PRINTER_IS_PRUSA_XL() && BOARD_IS_XLBUDDY()
    #include "Configuration_XL_adv.h"
#elif PRINTER_IS_PRUSA_XL() && BOARD_IS_DWARF()
    #include "Configuration_XL_Dwarf_adv.h"
#elif PRINTER_IS_PRUSA_MK4()
    #include "Configuration_MK4_adv.h"
#elif PRINTER_IS_PRUSA_MK3_5()
    #include "Configuration_MK3.5_adv.h"
#elif PRINTER_IS_PRUSA_iX()
    #include "Configuration_iX_adv.h"
#else
    #error "Unknown PRINTER_TYPE!"
#endif
