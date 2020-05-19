#include "version.h"
#include "config.h"

#define _STR(x) #x
#define STR(x)  _STR(x)

const int project_version_major = FW_VERSION_MAJOR;
const int project_version_minor = FW_VERSION_MINOR;
const int project_version_patch = FW_VERSION_PATCH;

const char project_version[] = STR(FW_VERSION);

const char project_version_full[] = STR(FW_VERSION_FULL);

const char project_version_suffix[] = STR(FW_VERSION_SUFFIX);

const char project_version_suffix_short[] = STR(FW_VERSION_SUFFIX_SHORT);

const int project_build_number = FW_BUILD_NUMBER;

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
const char project_firmware_name[] = "Buddy_MINI";
#else
    #error "unknown printer type"
#endif
