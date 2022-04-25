#pragma once

// The higher number the more flash is available

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI && defined(_DEBUG))
    #define SAVE_FLASH 10
#else
    #define SAVE_FLASH 0
#endif
