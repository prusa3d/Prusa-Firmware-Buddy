#pragma once
#include "printers.h"

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "main_MINI.h"
#else
    #error "Unknown PRINTER_TYPE!"
#endif
