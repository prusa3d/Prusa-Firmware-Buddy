//! @file
#pragma once

#include "config.h"

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "Configuration_A3ides_2209_MINI_adv.h"
#else
    #error "Unknown PRINTER_TYPE!"
#endif
