#include "version.h"
#include "config.h"

#define _STR(x) #x
#define STR(x)  _STR(x)

const char project_version[] = STR(FW_VERSION);

const char project_version_full[] = STR(FW_VERSION_FULL);

const char project_version_suffix[] = STR(FW_VERSION_SUFFIX);

const char project_version_suffix_short[] = STR(FW_VERSION_SUFFIX_SHORT);

const int project_build_number = FW_BUILD_NUMBER;

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
const char project_firmware_name[] = "Buddy_MINI";
#elif (PRINTER_TYPE == PRINTER_PRUSA_XL)
    #if (BOARD_IS_DWARF)
const char project_firmware_name[] = "Dwarf";
    #elif (BOARD_IS_MODULARBED)
const char project_firmware_name[] = "ModularBed";
    #else
const char project_firmware_name[] = "Buddy_XL";
    #endif
#elif (PRINTER_TYPE == PRINTER_PRUSA_MK4) || (PRINTER_TYPE == PRINTER_PRUSA_MK3_5)
const char project_firmware_name[] = "Buddy_MK4";
#elif (PRINTER_TYPE == PRINTER_PRUSA_iX)
const char project_firmware_name[] = "Buddy_iX";
#else
    #error "Unknown PRINTER_TYPE."
#endif
