//! @file
#pragma once

#include "config.h"
#include "save_flash_space.h"

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "Configuration_MINI_adv.h"
#else
    #error "Unknown PRINTER_TYPE!"
#endif
