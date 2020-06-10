#pragma once
#include "printers.h"

#if (!defined(PRINTER_PRUSA_MINI))
    #error "Some printer type not defined."
#endif
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "hwio_pindef_MINI.h"
#else
    #error "Unknown PRINTER_TYPE!"
#endif
