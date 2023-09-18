#include "version.h"
#include "config.h"
#include <string.h>

#define _STR(x) #x
#define STR(x)  _STR(x)

const char project_version[] = STR(FW_VERSION);

constexpr const uint16_t project_version_major = FW_VERSION_MAJOR;
constexpr const uint16_t project_version_minor = FW_VERSION_MINOR;
constexpr const uint16_t project_version_patch = FW_VERSION_PATCH;

const char project_version_full[] = STR(FW_VERSION_FULL);

const char project_version_suffix[] = STR(FW_VERSION_SUFFIX);

const char project_version_suffix_short[] = STR(FW_VERSION_SUFFIX_SHORT);

const int project_build_number = FW_BUILD_NUMBER;

#if PRINTER_IS_PRUSA_MINI
const char project_firmware_name[] = "Buddy_MINI";
#elif PRINTER_IS_PRUSA_XL
    #if (BOARD_IS_DWARF)
const char project_firmware_name[] = "Dwarf";
    #elif (BOARD_IS_MODULARBED)
const char project_firmware_name[] = "ModularBed";
    #else
const char project_firmware_name[] = "Buddy_XL";
    #endif
#elif PRINTER_IS_PRUSA_MK4
const char project_firmware_name[] = "Buddy_MK4";
#elif PRINTER_IS_PRUSA_MK3_5
const char project_firmware_name[] = "Buddy_MK3_5";
#elif PRINTER_IS_PRUSA_iX
const char project_firmware_name[] = "Buddy_iX";
#else
    #error "Unknown PRINTER_TYPE."
#endif

void fill_project_version_no_dots(char *buffer, size_t buffer_size) {
    for (size_t version_i = 0, buffer_i = 0; version_i < strlen(project_version) && buffer_i < buffer_size - 1; ++version_i) {
        if (project_version[version_i] == '\0') {
            buffer[buffer_i] = '\0';
            break;
        }
        if (project_version[version_i] == '.') {
            continue;
        }
        buffer[buffer_i] = project_version[version_i];
        ++buffer_i;
    }
}
